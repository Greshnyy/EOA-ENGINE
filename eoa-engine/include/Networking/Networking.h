#pragma once
#include "Core/Types.h"
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <unordered_map>
#include <queue>

namespace eoa {

// ============================================================================
// NETWORKING SYSTEM - Multiplayer replication
// ============================================================================

enum class NetworkRole : uint8_t {
    Authority,      // Сервер/хост имеет полный контроль
    AutonomousProxy,// Локальный игрок с prediction
    SimulatedProxy  // Remote игроки без prediction
};

enum class ConnectionStatus : uint8_t {
    Disconnected,
    Connecting,
    Connected,
    Reconnecting,
    Timeout
};

struct NetworkConfig {
    int serverPort = 7777;
    int maxClients = 16;
    float updateRate = 0.02f;        // 50 updates per second
    float timeoutSeconds = 30.0f;
    int maxPacketSize = 1400;
    bool enableLagCompensation = true;
    float clientPredictionTime = 0.1f;
};

struct Packet {
    uint32_t id = 0;
    std::vector<uint8_t> data;
    double timestamp = 0.0;
    int senderId = -1;
    bool reliable = true;
};

// ============================================================================
// NetworkPlayer - информация о подключенном игроке
// ============================================================================

struct NetworkPlayer {
    int id = -1;
    std::string username;
    std::string address;
    int port = 0;
    double connectedTime = 0.0;
    float ping = 0.0f;
    int packetsSent = 0;
    int packetsReceived = 0;
    bool isHost = false;
};

// ============================================================================
// ReplicatedProperty - свойство для репликации
// ============================================================================

template<typename T>
class ReplicatedProperty {
public:
    using ChangeCallback = std::function<void(const T& oldValue, const T& newValue)>;

    ReplicatedProperty() = default;
    explicit ReplicatedProperty(const T& value) : value_(value), lastSyncedValue_(value) {}

    const T& Get() const { return value_; }
    
    void Set(const T& value, bool forceReplicate = false) {
        if (forceReplicate || value_ != value) {
            T oldValue = value_;
            value_ = value;
            dirty_ = true;
            
            if (onChange_) {
                onChange_(oldValue, value);
            }
        }
    }

    bool IsDirty() const { return dirty_; }
    void MarkClean() { 
        dirty_ = false; 
        lastSyncedValue_ = value_; 
    }

    void SetOnChange(ChangeCallback callback) { onChange_ = callback; }

    // Operators
    operator const T&() const { return value_; }
    ReplicatedProperty& operator=(const T& value) { 
        Set(value); 
        return *this; 
    }

private:
    T value_{};
    T lastSyncedValue_{};
    bool dirty_ = false;
    ChangeCallback onChange_;
};

// ============================================================================
// NetworkChannel - канал для разных типов трафика
// ============================================================================

enum class ChannelType : uint8_t {
    ReliableOrdered,    // Гарантированная доставка по порядку (RPC, важные события)
    ReliableUnordered,  // Гарантированная доставка без порядка (файлы)
    UnreliableOrdered,  // Без гарантии, но по порядку (редкие обновления)
    UnreliableUnordered // Без гарантии и порядка (позиции, input)
};

class NetworkChannel {
public:
    explicit NetworkChannel(ChannelType type);
    
    ChannelType GetType() const { return type_; }
    
    void Send(const std::vector<uint8_t>& data);
    bool Receive(std::vector<uint8_t>& outData);
    
    void ProcessIncomingPackets();
    void Flush();

private:
    ChannelType type_;
    std::queue<Packet> incomingQueue_;
    std::queue<Packet> outgoingQueue_;
    std::vector<Packet> pendingAcks_;
};

// ============================================================================
// NetworkConnection - соединение с удаленным узлом
// ============================================================================

class NetworkConnection {
public:
    NetworkConnection(int id, const std::string& address, int port);
    ~NetworkConnection();

    int GetID() const { return id_; }
    const std::string& GetAddress() const { return address_; }
    int GetPort() const { return port_; }
    
    ConnectionStatus GetStatus() const { return status_; }
    void SetStatus(ConnectionStatus status) { status_ = status; }
    
    float GetPing() const { return ping_; }
    void UpdatePing(float ping) { ping_ = ping; }
    
    double GetLastActivity() const { return lastActivity_; }
    void Touch() { lastActivity_ = GetCurrentTime(); }
    
    NetworkChannel* GetChannel(ChannelType type);
    
    void SendPacket(const Packet& packet);
    bool ReceivePacket(Packet& outPacket);
    
    void Disconnect();

private:
    int id_;
    std::string address_;
    int port_;
    ConnectionStatus status_ = ConnectionStatus::Connecting;
    float ping_ = 0.0f;
    double lastActivity_ = 0.0;
    std::unordered_map<ChannelType, std::unique_ptr<NetworkChannel>> channels_;
};

// ============================================================================
// RPC - Remote Procedure Call
// ============================================================================

enum class RPCMode : uint8_t {
    Server,         // Вызвать только на сервере
    Client,         // Вызвать только на клиентах
    All,            // Вызвать у всех
    Owner,          // Вызвать только у владельца
    Others          // Вызвать у всех кроме локального
};

class RPCFunction {
public:
    using Callback = std::function<void(const std::vector<uint8_t>&)>;
    
    RPCFunction(const std::string& name, RPCMode mode, Callback callback)
        : name_(name), mode_(mode), callback_(callback) {}
    
    const std::string& GetName() const { return name_; }
    RPCMode GetMode() const { return mode_; }
    
    void Execute(const std::vector<uint8_t>& params) const {
        if (callback_) {
            callback_(params);
        }
    }

private:
    std::string name_;
    RPCMode mode_;
    Callback callback_;
};

// ============================================================================
// NetworkActor - компонент для сетевой репликации Actor
// ============================================================================

class NetworkActorComponent : public Component {
public:
    EOA_CLASS_DECL(NetworkActorComponent, Component)

    explicit NetworkActorComponent(const std::string& name = "NetworkActor");
    ~NetworkActorComponent() override;

    // Role и ownership
    NetworkRole GetLocalRole() const { return localRole_; }
    void SetLocalRole(NetworkRole role) { localRole_ = role; }
    
    int GetOwnerConnectionID() const { return ownerConnectionID_; }
    void SetOwnerConnectionID(int id) { ownerConnectionID_ = id; }

    // Реплицируемые свойства
    template<typename T>
    void AddReplicatedProperty(const std::string& name, ReplicatedProperty<T>* prop) {
        replicatedProps_[name] = [prop]() { return prop->IsDirty(); };
        replicatedPropsSerialize_[name] = [prop](std::vector<uint8_t>& data) {
            // Сериализация значения
            T value = prop->Get();
            auto* bytes = reinterpret_cast<uint8_t*>(&value);
            data.insert(data.end(), bytes, bytes + sizeof(T));
        };
        replicatedPropsDeserialize_[name] = [prop](const std::vector<uint8_t>& data) {
            // Десериализация значения
            T value;
            std::memcpy(&value, data.data(), sizeof(T));
            prop->Set(value);
        };
    }

    // RPC регистрация
    void RegisterRPC(const std::string& name, RPCMode mode, RPCFunction::Callback callback);
    void ExecuteRPC(const std::string& name, const std::vector<uint8_t>& params = {});

    // Tick для репликации
    void Tick(float deltaTime) override;

    // Сериализация состояния
    std::vector<uint8_t> SerializeState() const;
    void DeserializeState(const std::vector<uint8_t>& data);

private:
    NetworkRole localRole_ = NetworkRole::Authority;
    int ownerConnectionID_ = -1;
    
    struct ReplicatedPropBase {
        virtual ~ReplicatedPropBase() = default;
        virtual bool IsDirty() const = 0;
        virtual void MarkClean() = 0;
    };
    
    std::unordered_map<std::string, std::function<bool()>> replicatedProps_;
    std::unordered_map<std::string, std::function<void(std::vector<uint8_t>&)>> replicatedPropsSerialize_;
    std::unordered_map<std::string, std::function<void(const std::vector<uint8_t>&)>> replicatedPropsDeserialize_;
    std::unordered_map<std::string, RPCFunction> rpcFunctions_;
    
    float replicationAccumulator_ = 0.0f;
};

// ============================================================================
// NetworkManager - глобальный менеджер сети
// ============================================================================

class NetworkManager {
public:
    static NetworkManager& Get() {
        static NetworkManager instance;
        return instance;
    }

    // Инициализация/завершение
    bool Initialize(const NetworkConfig& config = NetworkConfig());
    void Shutdown();

    // Статус
    bool IsInitialized() const { return initialized_; }
    const NetworkConfig& GetConfig() const { return config_; }

    // Сервер функции
    bool StartServer(int port = -1);
    void StopServer();
    bool IsServer() const { return isServer_; }

    // Клиент функции
    bool ConnectToServer(const std::string& address, int port = -1);
    void DisconnectFromServer();
    bool IsClient() const { return connection_ != nullptr; }
    bool IsConnected() const;

    // Host (сервер + клиент одновременно)
    bool HostGame(int port = -1);

    // Отправка данных
    void SendToServer(const std::vector<uint8_t>& data, ChannelType channel = ChannelType::UnreliableUnordered);
    void SendToClient(int clientID, const std::vector<uint8_t>& data, ChannelType channel = ChannelType::UnreliableUnordered);
    void SendToAll(const std::vector<uint8_t>& data, ChannelType channel = ChannelType::UnreliableUnordered, int excludeClientID = -1);
    void SendToOthers(const std::vector<uint8_t>& data, ChannelType channel = ChannelType::UnreliableUnordered);

    // Получение данных
    bool ReceiveFromServer(std::vector<uint8_t>& outData);
    bool ReceiveFromClient(int clientID, std::vector<uint8_t>& outData);

    // Players
    const std::unordered_map<int, NetworkPlayer>& GetPlayers() const { return players_; }
    NetworkPlayer* GetPlayer(int id);
    NetworkPlayer* GetLocalPlayer() const;
    int GetLocalPlayerID() const { return localPlayerID_; }

    // Connections
    int GetConnectionCount() const { return static_cast<int>(connections_.size()); }
    NetworkConnection* GetConnection(int id);
    NetworkConnection* GetServerConnection() const { return connection_.get(); }

    // RPC
    void RegisterGlobalRPC(const std::string& name, RPCMode mode, RPCFunction::Callback callback);
    void ExecuteGlobalRPC(const std::string& name, const std::vector<uint8_t>& params = {});

    // Spawn/Despawn акторов
    int SpawnActor(Actor* actor, int ownerID = -1);
    void DespawnActor(int networkID);
    Actor* GetActorByNetworkID(int networkID) const;
    int GetNetworkIDForActor(Actor* actor) const;

    // Lag compensation
    void EnableLagCompensation(bool enabled) { config_.enableLagCompensation = enabled; }
    bool IsLagCompensationEnabled() const { return config_.enableLagCompensation; }

    // Статистика
    float GetServerFPS() const { return serverFPS_; }
    int GetBandwidthUp() const { return bandwidthUp_; }
    int GetBandwidthDown() const { return bandwidthDown_; }

    // Events
    using OnClientConnectedCallback = std::function<void(int clientID)>;
    using OnClientDisconnectedCallback = std::function<void(int clientID)>;
    using OnServerConnectedCallback = std::function<void()>;
    using OnServerDisconnectedCallback = std::function<void()>;

    void SetOnClientConnected(OnClientConnectedCallback callback) { onClientConnected_ = callback; }
    void SetOnClientDisconnected(OnClientDisconnectedCallback callback) { onClientDisconnected_ = callback; }
    void SetOnServerConnected(OnServerConnectedCallback callback) { onServerConnected_ = callback; }
    void SetOnServerDisconnected(OnServerDisconnectedCallback callback) { onServerDisconnected_ = callback; }

private:
    NetworkManager() = default;
    ~NetworkManager();

    NetworkConfig config_;
    bool initialized_ = false;
    bool isServer_ = false;

    std::unique_ptr<NetworkConnection> connection_;
    std::unordered_map<int, std::unique_ptr<NetworkConnection>> connections_;
    std::unordered_map<int, NetworkPlayer> players_;
    
    std::unordered_map<int, Actor*> networkIDToActor_;
    std::unordered_map<Actor*, int> actorToNetworkID_;
    int nextNetworkID_ = 1;

    int localPlayerID_ = -1;
    int nextClientID_ = 1;

    float serverFPS_ = 0.0f;
    int bandwidthUp_ = 0;
    int bandwidthDown_ = 0;

    std::unordered_map<std::string, RPCFunction> globalRPCs_;

    OnClientConnectedCallback onClientConnected_;
    OnClientDisconnectedCallback onClientDisconnected_;
    OnServerConnectedCallback onServerConnected_;
    OnServerDisconnectedCallback onServerDisconnected_;

    void Update(float deltaTime);
    void ProcessIncomingData();
    void HandleServerDiscovery();
    void SendHeartbeat();
};

// ============================================================================
// Утилиты для сериализации
// ============================================================================

namespace NetworkSerializer {
    template<typename T>
    void Write(std::vector<uint8_t>& buffer, const T& value) {
        auto* bytes = reinterpret_cast<const uint8_t*>(&value);
        buffer.insert(buffer.end(), bytes, bytes + sizeof(T));
    }

    template<typename T>
    T Read(const std::vector<uint8_t>& buffer, size_t& offset) {
        T value;
        std::memcpy(&value, buffer.data() + offset, sizeof(T));
        offset += sizeof(T);
        return value;
    }

    void WriteString(std::vector<uint8_t>& buffer, const std::string& str);
    std::string ReadString(const std::vector<uint8_t>& buffer, size_t& offset);

    void WriteVector3(std::vector<uint8_t>& buffer, const glm::vec3& vec);
    glm::vec3 ReadVector3(const std::vector<uint8_t>& buffer, size_t& offset);

    void WriteQuaternion(std::vector<uint8_t>& buffer, const glm::quat& quat);
    glm::quat ReadQuaternion(const std::vector<uint8_t>& buffer, size_t& offset);
}

// ============================================================================
// Макросы для удобной регистрации RPC
// ============================================================================

#define EOA_RPC_SERVER(funcName) \
    RegisterRPC(#funcName, RPCMode::Server, [this](const std::vector<uint8_t>& params) { funcName(params); })

#define EOA_RPC_CLIENT(funcName) \
    RegisterRPC(#funcName, RPCMode::Client, [this](const std::vector<uint8_t>& params) { funcName(params); })

#define EOA_RPC_ALL(funcName) \
    RegisterRPC(#funcName, RPCMode::All, [this](const std::vector<uint8_t>& params) { funcName(params); })

} // namespace eoa
