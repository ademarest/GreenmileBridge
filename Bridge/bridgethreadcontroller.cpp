#include "bridgethreadcontroller.h"

BridgeThreadController::BridgeThreadController(QObject *parent) : QObject(parent)
{
    Bridge *bridge = new Bridge;
    bridge->moveToThread(&bridgeThread_);
    connect(&bridgeThread_, &QThread::finished, bridge, &QObject::deleteLater);

    connect(bridge, &Bridge::started,               this, &BridgeThreadController::started);
    connect(bridge, &Bridge::aborted,               this, &BridgeThreadController::aborted);
    connect(bridge, &Bridge::rebuilt,               this, &BridgeThreadController::rebuilt);
    connect(bridge, &Bridge::finished,              this, &BridgeThreadController::finished);

    connect(bridge, &Bridge::bridgeKeyChanged,      this, &BridgeThreadController::bridgeKeyChanged);
    connect(bridge, &Bridge::bridgeProgress,        this, &BridgeThreadController::bridgeProgress);
    connect(bridge, &Bridge::currentJobProgress,    this, &BridgeThreadController::currentJobProgress);
    connect(bridge, &Bridge::currentJobChanged,     this, &BridgeThreadController::currentJobChanged);

    connect(bridge, &Bridge::debugMessage,          this, &BridgeThreadController::debugMessage);
    connect(bridge, &Bridge::errorMessage,          this, &BridgeThreadController::errorMessage);
    connect(bridge, &Bridge::statusMessage,         this, &BridgeThreadController::statusMessage);

    connect(this, &BridgeThreadController::addRequest,      bridge, &Bridge::addRequest);
    connect(this, &BridgeThreadController::removeRequest,   bridge, &Bridge::removeRequest);
    connect(this, &BridgeThreadController::abort,           bridge, &Bridge::abort);
    connect(this, &BridgeThreadController::hasActiveJobs,   bridge, &Bridge::hasActiveJobs);

    bridgeThread_.start();
}

BridgeThreadController::~BridgeThreadController()
{
    bridgeThread_.quit();
    bridgeThread_.wait();
}
