//
// Created by Mathw on 24/6/2026.
//

#include "network.h"
#include <iostream>
#include <optional>
#include <chrono>

Network::Network(int player_id) : player_id(player_id),
                                  client("http://127.0.0.1:3333"),
                                  isRunning(true) {
    client.set_connection_timeout(5);
    client.set_read_timeout(10);

    // Inicializa el thread y corre loop en segundo plano
    workerThread = std::thread(&Network::runLoop, this);
}

Network::~Network() {
    isRunning = false;

    // Espera a que el thread termine antes de destruir el objeto
    if (workerThread.joinable()) {
        workerThread.join();
    }
}

bool Network::fetchPlayer() {
    auto response = client.Get("/whoami");
    if (response && response->status == 200) {
        try {
            auto json = nlohmann::json::parse(response->body);
            player.id_players = json["id_players"];
            player.name = json["name"];
            player.email = json["email"];
            if (json.contains("age") && !json["age"].is_null()) {
                player.age = json["age"].get<int>();
            } else {
                player.age = std::nullopt;
            }
            player.roles = json["roles"].get<std::vector<std::string> >();
            return true;
        } catch (const std::exception &e) {
            std::cerr << "Failed to parse player info: " << e.what() << std::endl;
        }
    }
    return false;
}

bool Network::fetchMechanics() {
    auto response = client.Get("/videogames/12/mechanics");
    if (response && response->status == 200) {
        auto json = nlohmann::json::parse(response->body);

        // Crea un vector temporal para almacenar las mecánicas obtenidas
        std::vector<VideogameMechanic> fetchedMechanics;

        for (auto &m: json) {
            VideogameMechanic mechanic;
            mechanic.id_modifiable_mechanic_videogame = m["id_modifiable_mechanic_videogame"];
            mechanic.id_videogame = m["id_videogame"];
            mechanic.options = m["options"];
            mechanic.id_modifiable_mechanic = m["id_modifiable_mechanic"];
            mechanic.modifiable_mechanic_name = m["modifiable_mechanic_name"];
            if (m.contains("modifiable_mechanic_description") && !m["modifiable_mechanic_description"].is_null()) {
                mechanic.modifiable_mechanic_description = m["modifiable_mechanic_description"].get<std::string>();
            }
            if (m.contains("modifiable_mechanic_type") && !m["modifiable_mechanic_type"].is_null()) {
                mechanic.modifiable_mechanic_type = m["modifiable_mechanic_type"].get<std::string>();
            }
            // Agrega la mecánica obtenida al vector temporal
            fetchedMechanics.push_back(mechanic);
        }

        // Actualiza la lista de mecánicas de manera segura con mutex
        {
            std::lock_guard<std::mutex> lock(mutex);
            mechanics = std::move(fetchedMechanics);
        }
        return true;
    }
    std::cout << "Failed to fetch mechanics: " << (response
                                                       ? std::to_string(response->status)
                                                       : "No response from server") << std::endl;
    return false;
}

bool Network::fetchPlayerAttributes() {
    std::string path = "/players/" + std::to_string(player_id) + "/attributes/points";
    auto response = client.Get(path);
    if (response && response->status == 200) {
        try {
            auto json = nlohmann::json::parse(response->body);
            playerAttributePoints.clear();
            for (auto &item_json: json) {
                PlayerAttributePointsItem item;
                item.id_players = item_json["id_players"];
                item.player_name = item_json["player_name"];
                item.player_email = item_json["player_email"];
                item.id_attributes = item_json["id_attributes"];
                if (item_json.contains("attribute_name") && !item_json["attribute_name"].is_null()) {
                    item.attribute_name = item_json["attribute_name"].get<std::string>();
                } else {
                    item.attribute_name = std::nullopt;
                }
                item.balance_ledger = item_json["balance_ledger"];
                item.snapshot_points = item_json["snapshot_points"];
                item.diff_ledger_minus_snapshot = item_json["diff_ledger_minus_snapshot"];
                playerAttributePoints.push_back(item);
            }
            return true;
        } catch (const std::exception &e) {
            std::cerr << "Failed to parse player attributes: " << e.what() << std::endl;
        }
    }
    return false;
}


std::vector<VideogameMechanic> Network::getMechanics() const {
    std::lock_guard<std::mutex> lock(mutex);
    return mechanics;
}

const std::vector<PlayerAttributePointsItem> &Network::getPlayerAttributePoints() const {
    return playerAttributePoints;
}

void Network::printDebug() const {
    std::lock_guard<std::mutex> lock(mutex);
    std::cout << "--- Mechanics Info (Count: " << mechanics.size() << ") ---\n";
    for (const auto &mechanic: mechanics) {
        std::cout << "ID: " << mechanic.id_modifiable_mechanic << '\n';
        std::cout << "Name: " << mechanic.modifiable_mechanic_name << '\n';
        std::cout << "Description: " << (mechanic.modifiable_mechanic_description
                                             ? *mechanic.modifiable_mechanic_description
                                             : "null") << '\n';
        std::cout << "Type: " << (mechanic.modifiable_mechanic_type ? *mechanic.modifiable_mechanic_type : "null") <<
                '\n';
        std::cout << "Options: " << mechanic.options.dump(4) << '\n';
        std::cout << "------------------------\n";
    }
}

void Network::runLoop() {
    while (isRunning) {
        fetchMechanics();

        printDebug();

        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}
