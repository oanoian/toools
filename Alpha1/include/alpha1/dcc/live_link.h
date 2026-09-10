/**
 * Alpha1 Live Link
 * 
 * Real-time socket-based synchronization with DCC applications.
 * 
 * @file live_link.h
 */

#pragma once

#include "alpha1/core/types.h"
#include "alpha1/core/threading.h"
#include <string>
#include <functional>

namespace Alpha1::DCC {

using namespace Core;

/**
 * Live Link connection state
 */
enum class LiveLinkState {
    Disconnected,
    Connecting,
    Connected,
    Error
};

/**
 * Transform data from Live Link
 */
struct TransformData {
    float x, y, z;       // Position
    float qx, qy, qz, qw; // Rotation (quaternion)
    float sx, sy, sz;    // Scale
};

/**
 * Live Link for real-time DCC synchronization
 */
class LiveLink {
public:
    /**
     * Connect to DCC application
     */
    Result<void> Connect(const std::string& host, int port);
    
    /**
     * Disconnect from DCC application
     */
    void Disconnect();
    
    /**
     * Get current connection state
     */
    LiveLinkState GetState() const;
    
    /**
     * Subscribe to transform updates
     */
    void OnTransformUpdate(std::function<void(const std::string& name, const TransformData&)> callback);
    
    /**
     * Get latest transform for named object
     */
    Result<TransformData> GetTransform(const std::string& name) const;
    
private:
    LiveLinkState m_state = LiveLinkState::Disconnected;
    ConcurrentQueue<std::pair<std::string, TransformData>> m_transformQueue;
};

} // namespace Alpha1::DCC
