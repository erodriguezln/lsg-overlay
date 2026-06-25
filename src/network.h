//
// Created by Mathw on 24/6/2026.
//

#ifndef LSG_OVERLAY_NETWORK_H
#define LSG_OVERLAY_NETWORK_H

#include <httplib.h>
#include <nlohmann/json.hpp>
#include <optional>
#include <vector>
#include <string>

#include<mutex>
#include <thread>
#include <atomic>
#include<condition_variable>

struct PlayerResponse {
    int id_players = 0;
    std::string name;
    std::string email;
    std::optional<int> age;
    std::vector<std::string> roles; // "player", "teacher", "researcher", "admin", "developer"
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
    nlohmann::json options; // Objeto JSON flexible (por costo, dimensiones, etc.)
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

struct WatcherActiveGame {
    int idVideogame;
    int pid;
    std::optional<std::string> hwnd;
};

struct GameServerHealth {
    bool ok;
    std::optional<int> status;
};

struct RedeemRequest {
    int modifiable_mechanic_videogame_id;
    int point_dimension_id;
    int amount;
    nlohmann::json metadata;
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


class Network {
public:
    Network(int playerId, int videogameId);

    ~Network();

    bool fetchPlayer();

    bool fetchMechanics();

    bool fetchPlayerAttributes();

    bool fetchVideogameServerHealth();

    bool postReward(int rewardId);

    bool redeem(int mechanicId, int pointDimensionId, int amount, int rewardId);

    bool getIsVideogameServerOnline() const;

    PlayerResponse getPlayer() const;

    std::vector<VideogameMechanic> getMechanics() const;

    std::vector<PlayerAttributePointsItem> getPlayerAttributePoints() const;

    void printDebug() const;

private:
    int playerId;

    int videogameId;

    httplib::Client client;

    httplib::Client videogameServerClient;

    std::atomic<bool> isVideogameServerOnline{false};

    void runLoop();

    std::atomic<bool> isRunning{false};

    std::thread workerThread;

    mutable std::mutex mutex;

    std::condition_variable conditionalVariable;
    std::mutex conditionalVariableMutex;
    std::atomic<bool> forceRefresh{false};

    std::vector<VideogameMechanic> mechanics;
    PlayerResponse player;
    std::vector<PlayerAttributePointsItem> playerAttributePoints;
};


#endif //LSG_OVERLAY_NETWORK_H
