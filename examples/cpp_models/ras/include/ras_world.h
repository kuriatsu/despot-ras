#pragma once
#include "task_allocation.h"
#include "despot/interface/world.h"
#include "sumo_interface.h"
#include "operator_model.h"
#include "libgeometry.h"
#include "vehicle_model.h"

#include "nlohmann/json.hpp"

using namespace despot;

class RasWorld: public World {
private:
    TAState* pomdp_state; // save previous state
    std::vector<std::string> id_idx_list;
    std::vector<Risk> perception_targets;

    // log data
    nlohmann::json m_log = nlohmann::json::array();

public:
    SumoInterface* sim;
    OperatorModel* operator_model;
    TAValues* ta_values;

public:
    RasWorld();
    RasWorld(VehicleModel *vehicle_model, double delta_t, double obstacle_density, std::vector<double> perception_range); 
    bool Connect();
    State* Initialize();
    State* GetCurrentState();
    State* GetCurrentState(std::vector<double>& likelihood);
    bool ExecuteAction(ACT_TYPE action, OBS_TYPE& obs);
    void UpdateState(ACT_TYPE action, OBS_TYPE obs, const std::vector<double>& risk_probs);
    void Step();
    bool isTerminate();
    ~RasWorld();
}; 

