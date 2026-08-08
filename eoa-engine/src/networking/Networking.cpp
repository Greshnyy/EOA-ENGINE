#include "Networking/Networking.h"
#include "Core/World.h"
#include <chrono>
#include <cstring>

namespace eoa {

// ============================================================================
// NetworkChannel Implementation
// ============================================================================

NetworkChannel::NetworkChannel(ChannelType type) : type_(type) {}

void NetworkChannel::Send(const std::vector<uint8_t>& data) {
    Packet packet;
    packet.data = data;
    packet.timestamp = GetCurrentTime();
    outgoingQueue_.push(packet);
}

bool NetworkChannel::Receive(std::vector<uint8_t>& outData) {
    if (incomingQueue_.empty()) return false;
    
    const auto& packet = incomingQueue_.front();
    outData = packet.data;
    incomingQueue_.pop();
    return true;
}

void NetworkChannel::ProcessIncomingPackets() {
    // Обработка входящих пакетов в зависимости от типа канала
}

void NetworkChannel::Flush() {
    // Отправка всех накопленных пакетов
    while (!outgoingQueue_.empty()) {
        outgoingQueue_.pop();
    }
}

// ============================================================================
// NetworkConnection Implementation
// ============================================================================

NetworkConnection::NetworkConnection(int id, const std::string& address, int port)
    : id_(id), address_(address), port_(port) {
    channels_[ChannelType::ReliableOrdered] = std::make_unique<NetworkChannel>(ChannelType::ReliableOrdered);
    channels_[ChannelType::ReliableUnordered] = std::make_unique<NetworkChannel>(ChannelType::ReliableUnordered);
    channels_[ChannelType::UnreliableOrdered] = std::make_unique<NetworkChannel>(ChannelType::UnreliableOrdered);
    channels_[ChannelType::UnreliableUnordered] = std::make_unique<NetworkChannel>(ChannelType::UnreliableUnordered);
}

NetworkConnection::~NetworkConnection() {
    Disconnect();
}

NetworkChannel* NetworkConnection::GetChannel(ChannelType type) {
    auto it = channels_.find(type);
    return (it != channels_.end()) ? it->second.get() : nullptr;
}

void NetworkConnection::SendPacket(const Packet& packet) {
    Touch();
    // В полной реализации здесь будет отправка через сокет
}

bool NetworkConnection::ReceivePacket(Packet& outPacket) {
    // В полной реализации здесь будет получение из сокета
    return false;
}

void NetworkConnection::Disconnect() {
    status_ = ConnectionStatus::Disconnected;
    for (auto& [type, channel] : channels_) {
        channel->Flush();
    }
}

// ============================================================================
// NetworkActorComponent Implementation
// ============================================================================

NetworkActorComponent::NetworkActorComponent(const std::string& name)
    : Component(name) {
    EOA_CLASS_CONSTRUCT(NetworkActorComponent, Component)
}

NetworkActorComponent::~NetworkActorComponent() = default;

void NetworkActorComponent::RegisterRPC(const std::string& name, RPCMode mode, RPCFunction::Callback callback) {
    rpcFunctions_.emplace(name, RPCFunction(name, mode, callback));
}

void NetworkActorComponent::ExecuteRPC(const std::string& name, const std::vector<uint8_t>& params) {
    auto it = rpcFunctions_.find(name);
    if (it != rpcFunctions_.end()) {
        it->second.Execute(params);
    }
}

void NetworkActorComponent::Tick(float deltaTime) {
    replicationAccumulator_ += deltaTime;
    
    // Репликация свойств с заданной частотой
    float updateRate = NetworkManager::Get().GetConfig().updateRate;
    
    if (replicationAccumulator_ >= updateRate && localRole_ == NetworkRole::Authority) {
        // Проверка dirty свойств и отправка обновлений
        for (auto& [name, isDirtyFunc] : replicatedProps_) {
            if (isDirtyFunc()) {
                // Сериализация и отправка
                std::vector<uint8_t> data;
                replicatedPropsSerialize_[name](data);
                
                // Отправка всем клиентам
                NetworkManager::Get().SendToAll(data, ChannelType::UnreliableOrdered);
                
                // Очистка dirty флага
                replicatedProps_[name] = []() { return false; };
            }
        }
        
        replicationAccumulator_ = 0.0f;
    }
}

std::vector<uint8_t> NetworkActorComponent::SerializeState() const {
    std::vector<uint8_t> data;
    
    // Сериализация всех свойств
    for (const auto& [name, serializeFunc] : replicatedPropsSerialize_) {
        serializeFunc(data);
    }
    
    return data;
}

void NetworkActorComponent::DeserializeState(const std::vector<uint8_t>& data) {
    size_t offset = 0;
    
    // Десериализация всех свойств
    for (auto& [name, deserializeFunc] : replicatedPropsDeserialize_) {
        if (offset < data.size()) {
            std::vector<uint8_t> propData(data.begin() + offset, data.end());
            deserializeFunc(propData);
        }
    }
}

// ============================================================================
// NetworkManager Implementation
// ============================================================================

bool NetworkManager::Initialize(const NetworkConfig& config) {
    if (initialized_) return true;
    
    config_ = config;
    initialized_ = true;
    
    LOG_INFO("NetworkManager initialized");
    return true;
}

NetworkManager::~NetworkManager() {
    Shutdown();
}

void NetworkManager::Shutdown() {
    if (!initialized_) return;
    
    StopServer();
    DisconnectFromServer();
    
    connections_.clear();
    players_.clear();
    networkIDToActor_.clear();
    actorToNetworkID_.clear();
    
    initialized_ = false;
    LOG_INFO("NetworkManager shutdown");
}

bool NetworkManager::StartServer(int port) {
    if (!initialized_) return false;
    
    int serverPort = (port > 0) ? port : config_.serverPort;
    
    // В полной реализации здесь будет создание серверного сокета
    isServer_ = true;
    
    LOG_INFO("Server started on port {}", serverPort);
    return true;
}

void NetworkManager::StopServer() {
    if (!isServer_) return;
    
    // Отключение всех клиентов
    for (auto& [id, conn] : connections_) {
        conn->Disconnect();
    }
    
    connections_.clear();
    players_.clear();
    isServer_ = false;
    
    LOG_INFO("Server stopped");
}

bool NetworkManager::ConnectToServer(const std::string& address, int port) {
    if (!initialized_) return false;
    
    int serverPort = (port > 0) ? port : config_.serverPort;
    
    connection_ = std::make_unique<NetworkConnection>(0, address, serverPort);
    
    // В полной реализации здесь будет подключение к серверу
    connection_->SetStatus(ConnectionStatus::Connected);
    
    localPlayerID_ = nextClientID_++;
    
    NetworkPlayer player;
    player.id = localPlayerID_;
    player.address = address;
    player.port = serverPort;
    player.isHost = false;
    players_[localPlayerID_] = player;
    
    LOG_INFO("Connected to server at {}:{}", address, serverPort);
    
    if (onServerConnected_) {
        onServerConnected_();
    }
    
    return true;
}

void NetworkManager::DisconnectFromServer() {
    if (!connection_) return;
    
    connection_->Disconnect();
    connection_.reset();
    
    if (onServerDisconnected_) {
        onServerDisconnected_();
    }
    
    LOG_INFO("Disconnected from server");
}

bool NetworkManager::IsConnected() const {
    if (isServer_) return true;
    return connection_ && connection_->GetStatus() == ConnectionStatus::Connected;
}

bool NetworkManager::HostGame(int port) {
    if (!StartServer(port)) return false;
    
    // Создаем локального игрока как хоста
    localPlayerID_ = 0;
    
    NetworkPlayer hostPlayer;
    hostPlayer.id = 0;
    hostPlayer.isHost = true;
    players_[0] = hostPlayer;
    
    LOG_INFO("Hosting game on port {}", config_.serverPort);
    return true;
}

void NetworkManager::SendToServer(const std::vector<uint8_t>& data, ChannelType channel) {
    if (!connection_) return;
    
    auto* ch = connection_->GetChannel(channel);
    if (ch) {
        ch->Send(data);
        bandwidthUp_ += static_cast<int>(data.size());
    }
}

void NetworkManager::SendToClient(int clientID, const std::vector<uint8_t>& data, ChannelType channel) {
    auto it = connections_.find(clientID);
    if (it == connections_.end()) return;
    
    auto* ch = it->second->GetChannel(channel);
    if (ch) {
        ch->Send(data);
        bandwidthUp_ += static_cast<int>(data.size());
    }
}

void NetworkManager::SendToAll(const std::vector<uint8_t>& data, ChannelType channel, int excludeClientID) {
    for (auto& [id, conn] : connections_) {
        if (id != excludeClientID) {
            SendToClient(id, data, channel);
        }
    }
}

void NetworkManager::SendToOthers(const std::vector<uint8_t>& data, ChannelType channel) {
    SendToAll(data, channel, localPlayerID_);
}

bool NetworkManager::ReceiveFromServer(std::vector<uint8_t>& outData) {
    if (!connection_) return false;
    return connection_->ReceivePacket(*reinterpret_cast<Packet*>(&outData));
}

bool NetworkManager::ReceiveFromClient(int clientID, std::vector<uint8_t>& outData) {
    auto it = connections_.find(clientID);
    if (it == connections_.end()) return false;
    
    Packet packet;
    if (it->second->ReceivePacket(packet)) {
        outData = packet.data;
        bandwidthDown_ += static_cast<int>(packet.data.size());
        return true;
    }
    return false;
}

NetworkPlayer* NetworkManager::GetPlayer(int id) {
    auto it = players_.find(id);
    return (it != players_.end()) ? &it->second : nullptr;
}

NetworkPlayer* NetworkManager::GetLocalPlayer() const {
    return const_cast<NetworkManager*>(this)->GetPlayer(localPlayerID_);
}

NetworkConnection* NetworkManager::GetConnection(int id) {
    auto it = connections_.find(id);
    return (it != connections_.end()) ? it->second.get() : nullptr;
}

void NetworkManager::RegisterGlobalRPC(const std::string& name, RPCMode mode, RPCFunction::Callback callback) {
    globalRPCs_.emplace(name, RPCFunction(name, mode, callback));
}

void NetworkManager::ExecuteGlobalRPC(const std::string& name, const std::vector<uint8_t>& params) {
    auto it = globalRPCs_.find(name);
    if (it != globalRPCs_.end()) {
        it->second.Execute(params);
    }
}

int NetworkManager::SpawnActor(Actor* actor, int ownerID) {
    if (!actor) return -1;
    
    int networkID = nextNetworkID_++;
    networkIDToActor_[networkID] = actor;
    actorToNetworkID_[actor] = networkID;
    
    // Уведомление клиентов о спавне
    // В полной реализации здесь будет отправка RPC
    
    return networkID;
}

void NetworkManager::DespawnActor(int networkID) {
    auto it = networkIDToActor_.find(networkID);
    if (it == networkIDToActor_.end()) return;
    
    Actor* actor = it->second;
    actorToNetworkID_.erase(actor);
    networkIDToActor_.erase(it);
    
    // Уведомление клиентов о деспавне
}

Actor* NetworkManager::GetActorByNetworkID(int networkID) const {
    auto it = networkIDToActor_.find(networkID);
    return (it != networkIDToActor_.end()) ? it->second : nullptr;
}

int NetworkManager::GetNetworkIDForActor(Actor* actor) const {
    auto it = actorToNetworkID_.find(actor);
    return (it != actorToNetworkID_.end()) ? it->second : -1;
}

void NetworkManager::Update(float deltaTime) {
    if (!initialized_) return;
    
    // Обновление статистики
    static float fpsAccumulator = 0.0f;
    static int frameCount = 0;
    
    fpsAccumulator += deltaTime;
    frameCount++;
    
    if (fpsAccumulator >= 1.0f) {
        serverFPS_ = static_cast<float>(frameCount) / fpsAccumulator;
        fpsAccumulator = 0.0f;
        frameCount = 0;
        
        // Сброс bandwidth счетчиков каждую секунду
        bandwidthUp_ = 0;
        bandwidthDown_ = 0;
    }
    
    // Проверка таймаута клиентов
    if (isServer_) {
        double currentTime = GetCurrentTime();
        for (auto it = connections_.begin(); it != connections_.end();) {
            if (currentTime - it->second->GetLastActivity() > config_.timeoutSeconds) {
                int disconnectedID = it->first;
                it = connections_.erase(it);
                
                if (onClientDisconnected_) {
                    onClientDisconnected_(disconnectedID);
                }
                
                LOG_INFO("Client {} timed out", disconnectedID);
            } else {
                ++it;
            }
        }
    }
    
    ProcessIncomingData();
}

void NetworkManager::ProcessIncomingData() {
    // Обработка входящих данных от клиентов (на сервере)
    if (isServer_) {
        for (auto& [id, conn] : connections_) {
            Packet packet;
            while (conn->ReceivePacket(packet)) {
                // Обработка пакета
                players_[id].packetsReceived++;
            }
        }
    }
    
    // Обработка данных от сервера (на клиенте)
    if (connection_) {
        Packet packet;
        while (connection_->ReceivePacket(packet)) {
            // Обработка пакета
        }
    }
}

void NetworkManager::SendHeartbeat() {
    if (connection_) {
        std::vector<uint8_t> heartbeat = {0x01}; // Простой heartbeat пакет
        SendToServer(heartbeat, ChannelType::ReliableUnordered);
    }
}

// ============================================================================
// NetworkSerializer Implementation
// ============================================================================

void NetworkSerializer::WriteString(std::vector<uint8_t>& buffer, const std::string& str) {
    uint32_t length = static_cast<uint32_t>(str.size());
    Write(buffer, length);
    auto* bytes = reinterpret_cast<const uint8_t*>(str.c_str());
    buffer.insert(buffer.end(), bytes, bytes + length);
}

std::string NetworkSerializer::ReadString(const std::vector<uint8_t>& buffer, size_t& offset) {
    uint32_t length = Read<uint32_t>(buffer, offset);
    std::string str(reinterpret_cast<const char*>(buffer.data() + offset), length);
    offset += length;
    return str;
}

void NetworkSerializer::WriteVector3(std::vector<uint8_t>& buffer, const glm::vec3& vec) {
    Write(buffer, vec.x);
    Write(buffer, vec.y);
    Write(buffer, vec.z);
}

glm::vec3 NetworkSerializer::ReadVector3(const std::vector<uint8_t>& buffer, size_t& offset) {
    float x = Read<float>(buffer, offset);
    float y = Read<float>(buffer, offset);
    float z = Read<float>(buffer, offset);
    return glm::vec3(x, y, z);
}

void NetworkSerializer::WriteQuaternion(std::vector<uint8_t>& buffer, const glm::quat& quat) {
    Write(buffer, quat.w);
    Write(buffer, quat.x);
    Write(buffer, quat.y);
    Write(buffer, quat.z);
}

glm::quat NetworkSerializer::ReadQuaternion(const std::vector<uint8_t>& buffer, size_t& offset) {
    float w = Read<float>(buffer, offset);
    float x = Read<float>(buffer, offset);
    float y = Read<float>(buffer, offset);
    float z = Read<float>(buffer, offset);
    return glm::quat(w, x, y, z);
}

} // namespace eoa
