//
// Created by Mathw on 24/6/2026.
//

#include "network.h"
#include <iostream>
#include <optional>
#include <chrono>

Network::Network(int playerId, int videogameId) : playerId(playerId),
                                                    videogameId(videogameId),
                                                    client("http://127.0.0.1:3333"),
                                                    isRunning(true) {
    client.set_connection_timeout(5);
    client.set_read_timeout(10);

    // No cambia, asi que se llama solo una vez al inicio
    fetchPlayer();

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

    if (!response || response->status != 200) {
        return false;
    }

    try {
        auto json = nlohmann::json::parse(response->body);

        PlayerResponse fetchedPlayer;

        fetchedPlayer.id_players = json["id_players"];
        fetchedPlayer.name = json["name"];
        fetchedPlayer.email = json["email"];
        if (json.contains("age") && !json["age"].is_null()) {
            fetchedPlayer.age = json["age"].get<int>();
        } else {
            fetchedPlayer.age = std::nullopt;
        }

        fetchedPlayer.roles = json["roles"].get<std::vector<std::string> >();

        // Actualiza el player de manera segura con mutex
        {
            std::lock_guard<std::mutex> lock(mutex);
            player = std::move(fetchedPlayer);
        }

        return true;
    } catch (const std::exception &e) {
        std::cerr << "Failed to parse player info: " << e.what() << std::endl;
        return false;
    }
}

bool Network::fetchMechanics() {
    std::string path = "/videogames/" + std::to_string(videogameId) + "/mechanics";
    auto response = client.Get(path);

    if (!response || response->status != 200) {
        std::cout << "Failed to fetch mechanics: "
                << (response ? std::to_string(response->status) : "No response from server")
                << std::endl;
        return false;
    }

    try {
        auto json = nlohmann::json::parse(response->body);

        std::vector<VideogameMechanic> fetchedMechanics;

        for (const auto &m: json) {
            VideogameMechanic mechanic;

            mechanic.id_modifiable_mechanic_videogame =
                    m.at("id_modifiable_mechanic_videogame").get<int>();

            mechanic.id_videogame =
                    m.at("id_videogame").get<int>();

            mechanic.options =
                    m.at("options");

            mechanic.id_modifiable_mechanic =
                    m.at("id_modifiable_mechanic").get<int>();

            mechanic.modifiable_mechanic_name =
                    m.at("modifiable_mechanic_name").get<std::string>();

            if (m.contains("modifiable_mechanic_description") &&
                !m["modifiable_mechanic_description"].is_null()) {
                mechanic.modifiable_mechanic_description =
                        m["modifiable_mechanic_description"].get<std::string>();
            }

            if (m.contains("modifiable_mechanic_type") &&
                !m["modifiable_mechanic_type"].is_null()) {
                mechanic.modifiable_mechanic_type =
                        m["modifiable_mechanic_type"].get<std::string>();
            }

            fetchedMechanics.push_back(std::move(mechanic));
        }

        {
            std::lock_guard<std::mutex> lock(mutex);
            mechanics = std::move(fetchedMechanics);
        }

        return true;
    } catch (const std::exception &e) {
        std::cerr << "Failed to parse mechanics: " << e.what() << std::endl;
        return false;
    }
}

bool Network::fetchPlayerAttributes() {
    int target_id = 0;
    {
        std::lock_guard<std::mutex> lock(mutex);
        target_id = player.id_players;
    }
    if (target_id == 0) {
        target_id = playerId;
    }

    std::string path = "/players/" + std::to_string(target_id) + "/attributes/points";
    auto response = client.Get(path);

    if (!response || response->status != 200) {
        std::cerr << "Failed to fetch player attributes: "
                  << (response ? std::to_string(response->status) : "No response from server")
                  << " for path: " << path << std::endl;
        return false;
    }
    try {
        auto json = nlohmann::json::parse(response->body);

        std::vector<PlayerAttributePointsItem> fetchedItem;

        for (const auto &item_json: json) {
            PlayerAttributePointsItem item;
            
            // Check keys safely to prevent JSON type/key exceptions
            item.id_players = item_json.value("id_players", 0);
            item.player_name = item_json.value("player_name", "");
            item.player_email = item_json.value("player_email", "");
            item.id_attributes = item_json.value("id_attributes", 0);
            
            if (item_json.contains("attribute_name") && !item_json["attribute_name"].is_null()) {
                item.attribute_name = item_json["attribute_name"].get<std::string>();
            } else {
                item.attribute_name = std::nullopt;
            }
            
            item.balance_ledger = item_json.value("balance_ledger", 0.0);
            item.snapshot_points = item_json.value("snapshot_points", 0.0);
            item.diff_ledger_minus_snapshot = item_json.value("diff_ledger_minus_snapshot", 0.0);
            
            fetchedItem.push_back(item);
        }

        // Actualiza la lista de puntos de atributos del jugador de manera segura con mutex
        {
            std::lock_guard<std::mutex> lock(mutex);
            playerAttributePoints = std::move(fetchedItem);
        }

        return true;
    } catch (const std::exception &e) {
        std::cerr << "Failed to parse player attributes: " << e.what() << std::endl;
        return false;
    }
}

PlayerResponse Network::getPlayer() const {
    std::lock_guard<std::mutex> lock(mutex);
    return player;
}

std::vector<VideogameMechanic> Network::getMechanics() const {
    std::lock_guard<std::mutex> lock(mutex);
    return mechanics;
}

std::vector<PlayerAttributePointsItem> Network::getPlayerAttributePoints() const {
    std::lock_guard<std::mutex> lock(mutex);
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
        fetchPlayerAttributes();

        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}
