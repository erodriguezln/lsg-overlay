#include "network.h"
#include <iostream>
#include <optional>
#include <chrono>

// Inicializa los clientes HTTP y los hilos de fondo
Network::Network(int playerId, int videogameId) : playerId(playerId),
                                                  videogameId(videogameId),
                                                  client("http://127.0.0.1:3333"),
                                                  videogameServerClient("http://localhost:7878"),
                                                  isPeriodicQueryThreadRunning(true),
                                                  isRedeemThreadRunning(true) {
    // Limites de tiempo para evitar bloqueos si el backend no responde
    client.set_connection_timeout(5);
    client.set_read_timeout(10);

    // El servidor del videojuego puede estar apagado
    // Sus timeouts son mas cortos para no retrasar el hilo de consultas
    videogameServerClient.set_connection_timeout(2);
    videogameServerClient.set_read_timeout(2);

    // Lanza el hilo de consultas periódicas y el hilo de canjes
    periodicQueryThread = std::thread(&Network::periodicQueryLoop, this);
    redeemThread = std::thread(&Network::redeemLoop, this);
}

Network::~Network() {
    isPeriodicQueryThreadRunning = false;
    isRedeemThreadRunning = false;

    // Despierta el hilo de consultas para que pueda terminar
    periodicQueryWakeCv.notify_one();
    // Espera a que el hilo termine antes de destruir el objeto
    if (periodicQueryThread.joinable()) {
        periodicQueryThread.join();
    }

    // Despierta el hilo de canjes para que pueda terminar
    redeemRequestCv.notify_one();
    if (redeemThread.joinable()) {
        redeemThread.join();
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

        // Actualiza el jugador bajo mutex
        {
            std::lock_guard<std::mutex> lock(backendDataMutex);
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

        // Convierte cada elemento JSON a un struct
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

            // Campos opcionales
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

        // Reemplaza la lista bajo mutex para que la UI la lea de forma segura
        {
            std::lock_guard<std::mutex> lock(backendDataMutex);
            mechanics = std::move(fetchedMechanics);
        }

        return true;
    } catch (const std::exception &e) {
        std::cerr << "Failed to parse mechanics: " << e.what() << std::endl;
        return false;
    }
}

bool Network::fetchPlayerPointsBalance() {
    const std::string path = "/players/" + std::to_string(playerId) + "/points/balance";
    auto response = client.Get(path);

    if (!response || response->status != 200) {
        std::cerr << "Failed to fetch player points balance: "
                << (response ? std::to_string(response->status) : "No response from server")
                << " for path: " << path << std::endl;
        return false;
    }
    try {
        auto json = nlohmann::json::parse(response->body);

        std::vector<PlayerPointsBalanceItem> fetchedItems;

        for (const auto &item_json: json) {
            PlayerPointsBalanceItem item;

            // Usa valores por defecto para evitar errores por claves faltantes
            item.id_players = item_json.value("id_players", 0);
            item.id_point_dimension = item_json.value("id_point_dimension", 0);

            const auto readOptionalString = [&item_json](const char *key) -> std::optional<std::string> {
                if (item_json.contains(key) && !item_json[key].is_null()) {
                    return item_json[key].get<std::string>();
                }
                return std::nullopt;
            };

            const auto readOptionalInt = [&item_json](const char *key) -> std::optional<int> {
                if (item_json.contains(key) && !item_json[key].is_null()) {
                    return item_json[key].get<int>();
                }
                return std::nullopt;
            };

            item.dimension_code = readOptionalString("dimension_code");
            item.dimension_name = readOptionalString("dimension_name");
            item.id_attributes = readOptionalInt("id_attributes");
            item.attribute_name = readOptionalString("attribute_name");
            item.id_subattributes = readOptionalInt("id_subattributes");
            item.subattribute_name = readOptionalString("subattribute_name");
            item.balance = item_json.value("balance", 0.0);

            fetchedItems.push_back(std::move(item));
        }

        // Actualiza el balance del jugador bajo mutex
        {
            std::lock_guard<std::mutex> lock(backendDataMutex);
            playerPointsBalance = std::move(fetchedItems);
        }

        return true;
    } catch (const std::exception &e) {
        std::cerr << "Failed to parse player points balance: " << e.what() << std::endl;
        return false;
    }
}

bool Network::fetchVideogameServerHealth() {
    auto response = videogameServerClient.Get("/health");

    if (!response || response->status != 200) {
        std::cerr << "Failed to fetch videogame server health: "
                << (response ? std::to_string(response->status) : "No response from server")
                << std::endl;
        isModServerReachable.store(false);
        isGameAndServerReady.store(false);
        return false;
    }

    isModServerReachable.store(true);

    try {
        const auto json = nlohmann::json::parse(response->body);
        // El mod mientas carga puede responder 200 con game_ready: false
        const bool gameReady = json.value("game_ready", false);
        isGameAndServerReady.store(gameReady);
        return gameReady;
    } catch (const std::exception &e) {
        std::cerr << "Failed to parse health response: " << e.what() << std::endl;
        isGameAndServerReady.store(false);
        return false;
    }
}

bool Network::postReward(int rewardId) {
    nlohmann::json payload;
    payload["reward_id"] = rewardId;

    // Cliente local descartable para no compartir sockets entre hilos
    httplib::Client localVideogameServerClient("http://localhost:7878");
    localVideogameServerClient.set_connection_timeout(2);

    // Alinear con el timeout del servidor en el mod, de momento es 5 asi que lo dejamos en 6
    localVideogameServerClient.set_read_timeout(6);

    // Envia la mecanica elegida por POST al videojuego o mod para materializarla
    auto response = localVideogameServerClient.Post("/reward", payload.dump(), "application/json");

    if (!response || response->status != 200) {
        std::cerr << "Failed to send reward. ID: " << rewardId
                << ". Status: " << (response ? std::to_string(response->status) : "No response");
        if (response && !response->body.empty()) {
            std::cerr << ". Body: " << response->body;
        }
        std::cerr << std::endl;
        return false;
    }

    return true;
}

bool Network::redeem(int mechanicVideogameId, int attributeId, int amount, int rewardId) {
    // Protege la lectura de redeemState y la escritura de pendingRedeemTask
    std::lock_guard<std::mutex> lock(redeemRequestMutex);
    if (redeemState.load() == RedeemState::Processing || redeemState.load() == RedeemState::Pending) {
        return false;
    }

    pendingRedeemTask = {mechanicVideogameId, attributeId, amount, rewardId};

    redeemState.store(RedeemState::Pending);
    activeRedeemMechanicVideogameId.store(mechanicVideogameId);

    // Despierta el hilo de canjes para procesar la solicitud
    redeemRequestCv.notify_one();
    return true;
}


// TODO lsg-core puede aprobar el canje y el servidor del videojuego puede fallar al aplicar la recompensa
// Se podria implementar en lsg-core un mecanismo de rollback o reintento para manejar este caso.
// Esto lo documentare como limitaciones y mejoras futuras
bool Network::executeRedeem(int mechanicVideogameId, int attributeId, int amount, int rewardId) {
    nlohmann::json payload;
    payload["modifiable_mechanic_videogame_id"] = mechanicVideogameId;
    payload["attribute_id"] = attributeId;
    payload["amount"] = amount;
    payload["metadata"] = nullptr;

    std::string path = "/videogames/" + std::to_string(videogameId) + "/players/" + std::to_string(playerId) +
                       "/redeem";

    // Cliente local para evitar compartir sockets entre hilos
    // Al salir del scope se destruye y cierra el socket
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
            // Notifica la mecanica al videojuego o mod
            bool rewardPosted = postReward(rewardId);
            // Fuerza un refresh para que la UI muestre el nuevo balance
            periodicQueryRefreshRequested.store(true);
            // Despierta el hilo de consultas sin esperar los 5s
            periodicQueryWakeCv.notify_one();

            return rewardPosted;
        }
    } catch (const std::exception &e) {
        std::cerr << "Failed to parse redeem response: " << e.what() << std::endl;
    }

    return false;
}

// Indica si hay un canje pendiente o en proceso
bool Network::isCurrentlyProcessingOrPending() const {
    return redeemState.load() == RedeemState::Processing || redeemState.load() == RedeemState::Pending;
}

int Network::getCurrentRedeemMechanicVideogameId() const {
    return activeRedeemMechanicVideogameId.load();
}

PlayerResponse Network::getPlayer() const {
    std::lock_guard<std::mutex> lock(backendDataMutex);
    return player;
}

Videogame Network::getVideogame() const {
    std::lock_guard<std::mutex> lock(backendDataMutex);
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
            std::lock_guard<std::mutex> lock(backendDataMutex);
            videogame = std::move(fetchedVideogame);
        }

        return true;
    } catch (const std::exception &e) {
        std::cerr << "Failed to parse videogame info: " << e.what() << std::endl;
        return false;
    }
}

std::vector<VideogameMechanic> Network::getMechanics() const {
    std::lock_guard<std::mutex> lock(backendDataMutex);
    return mechanics;
}

std::vector<PlayerPointsBalanceItem> Network::getPlayerPointsBalance() const {
    std::lock_guard<std::mutex> lock(backendDataMutex);
    return playerPointsBalance;
}

bool Network::getIsGameAndServerReady() const {
    return isGameAndServerReady.load();
}

bool Network::getIsModServerReachable() const {
    return isModServerReachable.load();
}

void Network::periodicQueryLoop() {
    bool playerLoaded = false;
    bool videogameLoaded = false;

    while (isPeriodicQueryThreadRunning) {
        // Datos que solo se necesitan una vez al inicio
        if (!playerLoaded) {
            playerLoaded = fetchPlayer();
        }
        if (!videogameLoaded) {
            videogameLoaded = fetchVideogame();
        }

        // Datos que pueden cambiar en cada ciclo
        fetchMechanics();
        fetchPlayerPointsBalance();
        fetchVideogameServerHealth();

        // Espera cinco segundos o despierta antes si hay cierre o refresh forzado
        std::unique_lock<std::mutex> lock(periodicQueryWakeMutex);
        periodicQueryWakeCv.wait_for(lock, std::chrono::seconds(5), [this] {
            return !isPeriodicQueryThreadRunning.load() || periodicQueryRefreshRequested.load();
        });

        periodicQueryRefreshRequested.store(false);
    }
}

void Network::redeemLoop() {
    while (isRedeemThreadRunning.load()) {
        RedeemTask redeemRequest;
        {
            std::unique_lock<std::mutex> lock(redeemRequestMutex);
            // Espera hasta que haya un canje pendiente o se pida detener el hilo
            redeemRequestCv.wait(lock, [this] {
                return redeemState.load() == RedeemState::Pending || !isRedeemThreadRunning.load();
            });

            // Sale del loop si se pidio detener el hilo
            if (!isRedeemThreadRunning.load()) {
                break;
            }

            // Copia la tarea y marca el estado antes de soltar el lock
            // Esto asegura que redeem() no acepte otra solicitud mientras esta se ejecuta
            redeemRequest = pendingRedeemTask;
            redeemState.store(RedeemState::Processing);
        } // El lock se libera al salir del scope

        try {
            executeRedeem(redeemRequest.mechanicVideogameId, redeemRequest.attributeId, redeemRequest.amount,
                          redeemRequest.rewardId);
        } catch (const std::exception &e) {
            std::cerr << "Exception during redeem execution: " << e.what() << std::endl;
        }

        // Limpia el estado para permitir el siguiente canje
        activeRedeemMechanicVideogameId.store(-1);
        redeemState.store(RedeemState::Idle);
    }
}
