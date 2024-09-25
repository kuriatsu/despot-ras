#pragma once

#include <iostream>
#include <libsumo/libtraci.h>
#include <unordered_map>
#include <random>
#include <algorithm>
#include "libgeometry.h"
#include "vehicle_model.h"

using namespace libtraci;

class SumoInterface {
public:
    double m_delta_t;
    double m_density; // 1ppl per 1m
    std::vector<double> m_perception_range; // x l&r, y_forward

    VehicleModel *m_vehicle_model;
    std::string m_ego_name = "ego_vehicle";
    double m_risk_thresh = 0.5;

public:
    SumoInterface();
    SumoInterface(VehicleModel *vehicle_model, double delta_t, double density, std::vector<double> perception_range); 

    std::vector<Risk> perception();
    void controlEgoVehicle(const std::vector<Risk>& targets);
    void spawnPedestrians();
    void spawnEgoVehicle();
    double getEgoSpeed();
    Risk* getRisk(const std::string& id);
    std::vector<Risk> getRisk(const std::vector<std::string>& ids);
    void step(int delta_t = 0);
    void Run();
    void start();
    void close();
    void setColor(const std::string id, const std::vector<int> color, const std::string attrib) const;
    void log(double& time, Pose& ego_pose, std::vector<double>& other_ego_info, std::vector<Risk>& log_risks);
    bool isTerminate();
    
private:
    std::unordered_map<std::string, Risk> m_risks;

    // for logging
    std::vector<std::string> m_passed_targets;

};