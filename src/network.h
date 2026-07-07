#pragma once

#include <httplib.h>
#include <nlohmann/json.hpp>
#include <optional>
#include <vector>
#include <string>

#include<mutex>
#include <thread>
#include <atomic> // Variables compartidas sin bloqueo
#include<condition_variable>

struct PlayerResponse {
    int id_players = 0;
    std::string name;
    std::string email;
    std::optional<int> age;
    std::vector<std::string> roles;
};

struct Videogame {
    int id_videogame;
    std::string name;
    std::optional<std::string> genre;
    std::optional<std::string> description;
    std::optional<std::string> engine;
    std::optional<std::string> developer;
    std::optional<std::string> publisher;
    std::optional<std::string> launch;
    std::optional<std::string> version;
    std::optional<std::string> type;
    std::optional<std::string> executable;
};

struct VideogameMechanic {
    int id_modifiable_mechanic_videogame;
    int id_videogame;
    nlohmann::json options;
    int id_modifiable_mechanic;
    std::string modifiable_mechanic_name;
    std::optional<std::string> modifiable_mechanic_description;
    std::optional<std::string> modifiable_mechanic_type; // "buff", "nerf", "speed", etc.
};

struct PlayerAttributePointsItem {
    int id_players;
    std::string player_name;
    std::string player_email;
    int id_attributes;
    std::optional<std::string> attribute_name;
    double balance_ledger;
    double snapshot_points;
    double diff_ledger_minus_snapshot;
};

struct PlayerPointsBalanceItem {
    int id_players;
    int id_point_dimension;
    double balance;
};

struct GameServerHealth {
    bool ok;
    std::optional<int> status;
};

struct RedeemRequest {
    int modifiable_mechanic_videogame_id;
    int attribute_id;
    int amount;
    nlohmann::json metadata;
};

// Solicitud de canje procesada por el hilo de canjes
struct RedeemTask {
    int mechanicVideogameId;
    int attributeId;
    int amount;
    int rewardId;
};

struct RedeemResponse {
    std::string status;
    int points_ledger_id;
    std::string source_ref;
    double current_balance;
    double redeemed_amount;
    double resulting_balance;
    int game_id;
    int player_id;
    int point_dimension_id;
    int modifiable_mechanic_videogame_id;
};

// Estados de un canje
enum class RedeemState {
    Idle, // Sin operacion activa
    Pending, // Solicitud creada y esperando al hilo de canjes
    Processing, // Canje en ejecucion contra el backend
};


// Comunica el overlay con el backend local de LSG Client y con el servidor del videojuego/mod
// Mantiene copias locales de jugador videojuego mecanicas y puntos
// Usa hilos de fondo para refrescar datos y procesar canjes
// La UI debe leer los datos usando los getters thread safe
class Network {
public:
    // Crea los clientes HTTP y lanza los hilos de fondo
    Network(int playerId, int videogameId);

    // Señala a los hilos que deben detenerse y espera a que terminen
    ~Network();


    // GET /whoami - obtiene los datos del jugador de la sesión activa
    bool fetchPlayer();

    // GET /videogames/:id/mechanics - obtiene las mecánicas disponibles para el juego activo
    bool fetchMechanics();

    // GET /players/:id/attributes/points - obtiene el balance de puntos por atributo
    bool fetchPlayerAttributes();

    // GET /health (servidor del videojuego) - actualiza isVideogameServerOnline
    bool fetchVideogameServerHealth();

    // GET /videogames/:id - obtiene los datos del videojuego activo
    bool fetchVideogame();

    // POST /reward al servidor del videojuego - materializa la mecanica en el juego
    bool postReward(int rewardId);

    // Registra una solicitud de canje para que la procese el redeemWorkerThread
    // Retorna false si ya hay un canje en curso
    bool redeem(int mechanicId, int attributeId, int amount, int rewardId);

    // true si hay un canje pendiente o en curso
    bool isCurrentlyProcessingOrPending() const;

    // Retorna el ID de la mecanica en proceso o -1 si no hay ninguna activa
    int getCurrentRedeemMechanicVideogameId() const;

    // true si el servidor del videojuego respondió correctamente en el último health check
    bool getIsVideogameServerOnline() const;

    // Getters thread-safe adquieren el mutex internamente y retornan una copia del dato
    PlayerResponse getPlayer() const;

    Videogame getVideogame() const;

    std::vector<VideogameMechanic> getMechanics() const;

    std::vector<PlayerAttributePointsItem> getPlayerAttributePoints() const;

    void printDebug() const;

    // ID de la mecanica en proceso de canje
    // Es atomic para que la UI pueda leerlo sin bloqueo
    std::atomic<int> currentRedeemMechanicId{-1};

private:
    int playerId;
    int videogameId;

    // Backend local de LSG Client
    httplib::Client client;
    // Servidor del videojuego o mod solo para health check
    httplib::Client videogameServerClient;

    // Permite leer un solo valor desde cualquier hilo sin mutex
    std::atomic<bool> isVideogameServerOnline{false};

    // Datos leidos por la UI y actualizados por el hilo de consultas
    // Deben accederse bajo mutex o mediante getters
    std::vector<VideogameMechanic> mechanics;
    PlayerResponse player;
    std::vector<PlayerAttributePointsItem> playerAttributePoints;
    Videogame videogame;

    // Hilo de consultas periodicas
    void runLoop();

    mutable std::mutex mutex; // Protege player videogame mechanics y playerAttributePoints
    std::mutex conditionalVariableMutex;
    std::condition_variable conditionalVariable; // Permite dormir y despertar al hilo
    std::thread workerThread;
    std::atomic<bool> isRunning{false}; // false indica al workerThread que debe terminar
    std::atomic<bool> forceRefresh{false}; // true despierta el hilo antes de los 5s

    // Hilo de canjes
    void redeemWorkerLoop();

    bool executeRedeem(int mechanicId, int attributeId, int amount, int rewardId); // Lógica interna del canje
    RedeemTask pendingRedeem; // Solicitud de canje registrada por redeem
    std::atomic<RedeemState> redeemState{RedeemState::Idle}; // Estado actual del canje
    mutable std::mutex redeemMutex; // Protege pendingRedeem y redeemState
    std::condition_variable redeemCV; // Permite dormir y despertar al hilo
    std::atomic<bool> isRedeemWorkerRunning{true}; // false indica al redeemWorkerThread que debe terminar
    std::thread redeemWorkerThread;
};
