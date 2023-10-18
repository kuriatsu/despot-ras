#include "sumo_interface.h"

using namespace libtraci;

SumoInterface::SumoInterface() {
    m_delta_t = 1.0;
    m_density = 0.1;
    m_perception_range = {50, 150};
    m_vehicle_model = new VehicleModel();
}

SumoInterface::SumoInterface(VehicleModel *vehicle_model, double delta_t, double density, std::vector<double> perception_range) {
    m_vehicle_model = vehicle_model;
    m_density = density;
    m_delta_t = delta_t;
    m_perception_range = perception_range;
}

std::vector<std::string> SumoInterface::perception() {

    Pose ego_pose;
    try { 
        ego_pose = Pose(m_ego_name, "v");
    }
    catch (libsumo::TraCIException& error) {
        std::cout << "no ego_vehicle" << std::endl;
        Simulation::close();
    }
    // std::vector<std::string> ego_route = Vehicle::getRoute(m_ego_name);
    
    std::vector<std::string> perception_target_ids;
    // m_passed_targets.clear();

    for (std::string& ped_id : Person::getIDList()) {
        
        // get obstacles along to the ego vehicle route
        // if (std::find(ego_route.begin(), ego_route.end(), ped_id_edge) == ego_route.end()) continue;

        // remove peds which is on the edge of the lane
        std::string ped_edge = Person::getRoadID(ped_id);
        std::string ped_lane_id;
        for (const auto& lane_id : Lane::getIDList()) {
            if (Lane::getEdgeID(lane_id) == ped_edge) { 
                ped_lane_id = lane_id;
            }
        }
        if (Person::getLanePosition(ped_id) == Lane::getLength(ped_lane_id) || Person::getLanePosition(ped_id) == 0.0) {
            continue;
        }


        // get risk position
        m_risks[ped_id].pose = Pose(ped_id, "p");
        Pose rel_risk_pose = m_risks[ped_id].pose.transformTo(ego_pose);
        double prev_distance = m_risks[ped_id].distance;
        m_risks[ped_id].distance = rel_risk_pose.y;

        if (fabs(rel_risk_pose.x) < m_perception_range[0]/2 && 0 < rel_risk_pose.y && rel_risk_pose.y < m_perception_range[1]) {
            Person::setColor(ped_id, libsumo::TraCIColor(200, 200, 0));
            perception_target_ids.emplace_back(ped_id);
        }
        else {
            Person::setColor(ped_id, libsumo::TraCIColor(0, 0, 200));
        }
    }
    return perception_target_ids;
}

void SumoInterface::setColor(const std::string id, const std::vector<int> color, const std::string attrib) const {
    if (color.size() < 3) {
        std::cerr << "color size" << sizeof(color)/sizeof(int) << " is less than 3" << std::endl;
        return;
    }

    if (attrib == "v" || attrib == "vehicle") {
        Vehicle::setColor(id, libsumo::TraCIColor(color[0], color[1], color[2]));
    }
    else if (attrib == "p" || attrib == "person") {
        Person::setColor(id, libsumo::TraCIColor(color[0], color[1], color[2]));
    }
}

void SumoInterface::controlEgoVehicle(const std::vector<int>& target_poses, const std::vector<bool> target_risks) const {

    double speed;
    try {
        speed = Vehicle::getSpeed(m_ego_name);
    }
    catch (libsumo::TraCIException& error) {
        // std::cout << "no ego_vehicle" << std::endl;
        Simulation::close();
    }

    double a = m_vehicle_model->getAccel(speed, 0, target_risks, target_poses);
    a = m_vehicle_model->clipSpeed(a, speed);
    Vehicle::setAcceleration(m_ego_name, a, m_delta_t);

}

void SumoInterface::spawnEgoVehicle() {
    std::cout << "spawn ego vehicle" << std::endl;
    auto edge_list = Edge::getIDList();
    auto route_list = Route::getIDList(); 
    if (Route::getIDList().empty()) {
        Route::add("ego_vehicle_route", {"E0"});
        // std::cout << "add new route: " << Route::getIDList()[0] << std::endl;
    }
    
    // for (auto &itr : Route::getIDList()){
    //     std::cout << itr << std::endl;
    // }

    Vehicle::add("ego_vehicle", Route::getIDList()[0]);
    Vehicle::setColor("ego_vehicle", libsumo::TraCIColor(0, 200, 0));
    Vehicle::setMaxSpeed("ego_vehicle", m_vehicle_model->m_max_speed);
    Vehicle::setAccel("ego_vehicle", m_vehicle_model->m_max_accel);
    Vehicle::setDecel("ego_vehicle", m_vehicle_model->m_max_decel);
    std::cout << "spawned ego vehicle" << std::endl;

    // GUI::track(m_ego_name, "View #0");
}

void SumoInterface::spawnPedestrians() {
    std::cout << "spawn pedestrians" << m_density << std::endl;
    if (m_density == 0.0) {
        std::cout << "pedestrian dencity is 0" << std::endl;
        return;
    }

    double interval = 1/m_density;

    // Generate random value
    std::mt19937 mt{std::random_device{}()};
    std::uniform_real_distribution<double> position_noise(-interval, interval), prob(0, 1), rand(0, 1);

    // add peds for each lane
    auto lane_list = Lane::getIDList();
    for (std::string& lane_id : lane_list) {

        const auto& allowed_list = Lane::getAllowed(lane_id);
        if (!std::count(allowed_list.begin(), allowed_list.end(),"pedestrian")) continue;

        std::string edge = Lane::getEdgeID(lane_id);
        double lane_length = Lane::getLength(lane_id);

        // add peds
        for (int i=0; i<lane_length; i+=(int)interval) {
            double position = i + position_noise(mt);
            if (std::abs(position) > lane_length) continue;
            std::string ped_id = lane_id + "-" + std::to_string(i);
            Person::add(ped_id, edge, position);
            Person::setColor(ped_id, libsumo::TraCIColor(0, 0, 200));
            Person::appendWalkingStage(ped_id, {edge}, 0);
            Person::appendWaitingStage(ped_id, 1000);
            Person::setSpeed(ped_id, 0.8);
            double risk_prob = prob(mt);
            bool risk = (rand(mt) < risk_prob) ? true : false;
            m_risks[ped_id] = Risk(ped_id, risk, risk_prob); 
        }
    }
    std::cout << "spawned pedestrian" << m_risks.size() << std::endl;
}

void SumoInterface::spawnPedestrians(std::vector<Risk> obj_list) {
    std::cout << "spawn pedestrians from log file" << m_density << std::endl;

    // add peds
    for (auto risk : obj_list) {
        Person::add(risk.id, risk.pose.lane, risk.pose.lane_position);
        Person::setColor(risk.id, libsumo::TraCIColor(0, 0, 200));
        Person::appendWalkingStage(risk.id, {risk.pose.lane}, 0);
        Person::appendWaitingStage(risk.id, 1000);
        Person::setSpeed(risk.id, 0.8);
        m_risks[risk.id] = risk; 
    }
    std::cout << "spawned pedestrian" << m_risks.size() << std::endl;
}

double SumoInterface::getEgoSpeed() {
    return Vehicle::getSpeed(m_ego_name);
}

Risk* SumoInterface::getRisk(const std::string& id){
    return &m_risks[id];
}

std::vector<Risk> SumoInterface::getRisk(const std::vector<std::string>& ids){
    std::vector<Risk> out_list;
    for (const auto& id : ids) {
        out_list.emplace_back(m_risks[id]);
    }
    return out_list;
}


void SumoInterface::log(double& time, Pose& ego_pose, std::vector<double>& other_ego_info, std::vector<Risk>& log_risks) {
// NOTE : vehicle_info = [pose x, pose y, speed, accel, fuel_consumption]
// NOTE : passed_risks = [likelihood, risk_pred, risk_hidden, likelihood, ...]
    ego_pose = Pose(m_ego_name, "v");
    other_ego_info.emplace_back(Vehicle::getSpeed(m_ego_name));
    other_ego_info.emplace_back(Vehicle::getAcceleration(m_ego_name));
    other_ego_info.emplace_back(Vehicle::getFuelConsumption(m_ego_name));
    
    for (const auto risk : m_risks) {
    // for (const auto id : m_passed_targets) {
    //    passed_risks.emplace_back(m_risks[id]);
        log_risks.emplace_back(risk.second);
    }
}

void SumoInterface::step(int delta_t) {

    Simulation::step(delta_t);
}

void SumoInterface::close() {
    Simulation::close();
}

void SumoInterface::start() {
    // Simulation::start({"sumo", "-c", "map/straight.sumocfg"});
    Simulation::start({"sumo", "-c", "../map/straight.sumocfg"});
    // Simulation::start({"sumo-gui", "-c", "../map/straight.sumocfg"});
    // try {
    //     Simulation::load({"-c", "../map/straight.sumocfg"});
    // }
    // catch (libsumo::TraCIException& error) {
    // }
    // catch (libsumo::FatalTraCIError& error) {
    //     Simulation::start({"sumo", "-c", "../map/straight.sumocfg"});
    // }
    // Simulation::executeMove();
//    if (Simulation::hasGUI()) {
//        Simulation::load({"-c", "../map/straight.sumocfg"});
//    }
//    else {
//        Simulation::start({"sumo-gui", "-c", "../map/straight.sumocfg"});
//        // Simulation::start({"sumo-gui", "-r", "./straight.net.xml"});
//    }
}

void SumoInterface::Run() {
    Simulation::executeMove();
}


bool SumoInterface::isTerminate() {
    try { 
        Pose(Vehicle::getPosition(m_ego_name));
    }
    catch (libsumo::TraCIException& error) {
        std::cout << "end of simulation" << std::endl;
        return true;
    }
    return false;
}

/*
int main(int argc, char* argv[]) {
    Simulation::start({"sumo-gui", "-c", "straight.sumocfg"});
    // Simulation::start({"sumo-gui", "-r", "./straight.net.xml"});
    auto sim = SumoInterface();
    sim.spawnPedestrians();
    sim.spawnEgoVehicle();

    for (int i = 0; i < 3000; i++) {
        Simulation::step();
        auto targets = sim.perception();
        sim.controlEgoVehicle(targets);
    Simulation::close();
}
*/
