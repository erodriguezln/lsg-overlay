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
                                                  videogameServerClient("http://localhost:7878"),
                                                  isRunning(true),
                                                  isRedeemWorkerRunning(true) {
    // Límites de tiempo para evitar bloquear el thread si las conexiones no responden
    client.set_connection_timeout(5);
    client.set_read_timeout(10);

    videogameServerClient.set_connection_timeout(2);
    videogameServerClient.set_read_timeout(2);

    // Inicializa el thread y corre loop en segundo plano
    workerThread = std::thread(&Network::runLoop, this);

    redeemWorkerThread = std::thread(&Network::redeemWorkerLoop, this);
}

Network::~Network() {
    isRunning = false;
    isRedeemWorkerRunning = false;

    // Notifica al thread para que despierte y termine
    conditionalVariable.notify_one();
    // Espera a que el thread termine antes de destruir el objeto
    if (workerThread.joinable()) {
        workerThread.join();
    }

    redeemCV.notify_one();
    if (redeemWorkerThread.joinable()) {
        redeemWorkerThread.join();
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
    std::string path = "/players/" + std::to_string(playerId) + "/attributes/points";
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

bool Network::fetchVideogameServerHealth() {
    auto response = videogameServerClient.Get("/health");

    if (!response || response->status != 200) {
        std::cerr << "Failed to fetch videogame server health: "
                << (response ? std::to_string(response->status) : "No response from server")
                << std::endl;
        // Inicialmente marca el servidor del videojuego como offline
        isVideogameServerOnline.store(false);
        return false;
    }

    // Marca el servidor como online
    isVideogameServerOnline.store(true);
    return true;
}

bool Network::postReward(int rewardId) {
    nlohmann::json payload;
    payload["reward_id"] = rewardId;

    httplib::Client localVideogameServerClient("http://localhost:7878");
    localVideogameServerClient.set_connection_timeout(2);
    localVideogameServerClient.set_read_timeout(2);

    // Envía la mecanica elegida por POST para materializarla en el videojuego/mod
    auto response = localVideogameServerClient.Post("/reward", payload.dump(), "application/json");

    if (!response || response->status != 200) {
        std::cerr << "Failed to send reward. ID: " << rewardId
                << ". Status: " << (response ? std::to_string(response->status) : "No response")
                << std::endl;
        return false;
    }

    return true;
}

bool Network::redeem(int mechanicId, int attributeId, int amount, int rewardId) {
    std::lock_guard<std::mutex> lock(redeemMutex);
    if (redeemState.load() == RedeemState::Processing || redeemState.load() == RedeemState::Pending) {
        return false;
    }

    pendingRedeem = {mechanicId, attributeId, amount, rewardId};

    redeemState.store(RedeemState::Pending);
    currentRedeemMechanicId.store(mechanicId);

    redeemCV.notify_one();
    return true;
}

bool Network::executeRedeem(int mechanicId, int attributeId, int amount, int rewardId) {
    nlohmann::json payload;
    payload["modifiable_mechanic_videogame_id"] = mechanicId;
    payload["attribute_id"] = attributeId;
    payload["amount"] = amount;
    payload["metadata"] = nullptr;

    std::string path = "/videogames/" + std::to_string(videogameId) + "/players/" + std::to_string(playerId) +
                       "/redeem";

    // Se usa un cliente local para evitar colisiones de socket (race conditions) con el thread secundario de fondo, ya que httplib::Client no es thread-safe.
    // Al salir de scope se destruye automáticamente cerrando el socket.
    httplib::Client localClient("http://127.0.0.1:3333");
    localClient.set_connection_timeout(5);
    localClient.set_read_timeout(10);

    auto response = localClient.Post(path.c_str(), payload.dump(), "application/json");

    if (!response || response->status != 200) {
        std::cerr << "Failed to redeem mechanic. Status: " << (response
                                                                   ? std::to_string(response->status)
                                                                   : "No response") << std::endl;
        return false;
    }

    try {
        auto jsonResponse = nlohmann::json::parse(response->body);
        std::string status = jsonResponse.value("status", "");
        if (status == "redeemed") {
            // Notificar recompensa externa en el videojuego/mod
            bool rewardPosted = postReward(rewardId);
            // Actualizar los atributos del jugador para reflejar el nuevo balance de puntos
            forceRefresh.store(true);
            // Notificar al thread de fondo que se ha producido un cambio que requiere actualización
            conditionalVariable.notify_one();

            return rewardPosted;
        }
    } catch (const std::exception &e) {
        std::cerr << "Failed to parse redeem response: " << e.what() << std::endl;
    }

    return false;
}

// Getter para saber si actualmente se está ejecutando un proceso de redeem en segundo plano
bool Network::isCurrentlyProcessingOrPending() const {
    return redeemState.load() == RedeemState::Processing || redeemState.load() == RedeemState::Pending;
}

int Network::getCurrentRedeemMechanicId() const {
    return currentRedeemMechanicId.load();
}


PlayerResponse Network::getPlayer() const {
    std::lock_guard<std::mutex> lock(mutex);
    return player;
}

Videogame Network::getVideogame() const {
    std::lock_guard<std::mutex> lock(mutex);
    return videogame;
}

bool Network::fetchVideogame() {
    std::string path = "/videogames/" + std::to_string(videogameId);
    auto response = client.Get(path);

    if (!response || response->status != 200) {
        std::cerr << "Failed to fetch videogame info: "
                << (response ? std::to_string(response->status) : "No response from server")
                << std::endl;
        return false;
    }

    try {
        auto json = nlohmann::json::parse(response->body);

        Videogame fetchedVideogame;

        fetchedVideogame.id_videogame = json["id_videogame"];
        fetchedVideogame.name = json["name"];

        if (json.contains("genre") && !json["genre"].is_null()) {
            fetchedVideogame.genre = json["genre"].get<std::string>();
        } else {
            fetchedVideogame.genre = std::nullopt;
        }

        if (json.contains("description") && !json["description"].is_null()) {
            fetchedVideogame.description = json["description"].get<std::string>();
        } else {
            fetchedVideogame.description = std::nullopt;
        }

        if (json.contains("engine") && !json["engine"].is_null()) {
            fetchedVideogame.engine = json["engine"].get<std::string>();
        } else {
            fetchedVideogame.engine = std::nullopt;
        }

        if (json.contains("developer") && !json["developer"].is_null()) {
            fetchedVideogame.developer = json["developer"].get<std::string>();
        } else {
            fetchedVideogame.developer = std::nullopt;
        }

        if (json.contains("publisher") && !json["publisher"].is_null()) {
            fetchedVideogame.publisher = json["publisher"].get<std::string>();
        } else {
            fetchedVideogame.publisher = std::nullopt;
        }

        if (json.contains("launch") && !json["launch"].is_null()) {
            fetchedVideogame.launch = json["launch"].get<std::string>();
        } else {
            fetchedVideogame.launch = std::nullopt;
        }

        if (json.contains("version") && !json["version"].is_null()) {
            fetchedVideogame.version = json["version"].get<std::string>();
        } else {
            fetchedVideogame.version = std::nullopt;
        }

        if (json.contains("type") && !json["type"].is_null()) {
            fetchedVideogame.type = json["type"].get<std::string>();
        } else {
            fetchedVideogame.type = std::nullopt;
        }

        if (json.contains("executable") && !json["executable"].is_null()) {
            fetchedVideogame.executable = json["executable"].get<std::string>();
        } else {
            fetchedVideogame.executable = std::nullopt;
        }

        {
            std::lock_guard<std::mutex> lock(mutex);
            videogame = std::move(fetchedVideogame);
        }

        return true;
    } catch (const std::exception &e) {
        std::cerr << "Failed to parse videogame info: " << e.what() << std::endl;
        return false;
    }
}

std::vector<VideogameMechanic> Network::getMechanics() const {
    std::lock_guard<std::mutex> lock(mutex);
    return mechanics;
}

std::vector<PlayerAttributePointsItem> Network::getPlayerAttributePoints() const {
    std::lock_guard<std::mutex> lock(mutex);
    return playerAttributePoints;
}

bool Network::getIsVideogameServerOnline() const {
    return isVideogameServerOnline.load();
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
    bool playerLoaded = false;
    bool videogameLoaded = false;

    while (isRunning) {
        if (!playerLoaded) {
            playerLoaded = fetchPlayer();
        }
        if (!videogameLoaded) {
            videogameLoaded = fetchVideogame();
        }

        fetchMechanics();
        fetchPlayerAttributes();
        // TODO: Podria dejar esto en otro thread...
        fetchVideogameServerHealth();

        std::unique_lock<std::mutex> lock(conditionalVariableMutex);
        conditionalVariable.wait_for(lock, std::chrono::seconds(5), [this] {
            return !isRunning.load() || forceRefresh.load();
        });

        forceRefresh.store(false);
    }
}

void Network::redeemWorkerLoop() {
    while (isRedeemWorkerRunning.load()) {
        RedeemTask request;
        {
            std::unique_lock<std::mutex> lock(redeemMutex);
            // Espera hasta que haya un request pendiente o se indique que se debe detener el thread
            redeemCV.wait(lock, [this] {
                return redeemState.load() == RedeemState::Pending || !isRedeemWorkerRunning.load();
            });

            // Sale del loop si se indica que se debe detener el thread
            if (!isRedeemWorkerRunning.load()) {
                break;
            }

            request = pendingRedeem;
            redeemState.store(RedeemState::Processing);
        }

        try {
            executeRedeem(request.mechanicVideogameId, request.attributeId, request.amount, request.rewardId);
        } catch (const std::exception &e) {
            std::cerr << "Exception during redeem execution: " << e.what() << std::endl;
        }

        currentRedeemMechanicId.store(-1);
        redeemState.store(RedeemState::Idle);
    }
}
