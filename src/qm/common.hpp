// Shared types for the ecce-qm library.  No wx, no ECCE dependencies.
#pragma once
#include <Eigen/Dense>
#include <stdexcept>
#include <string>

namespace qm {

using Mat = Eigen::MatrixXd;
using Vec = Eigen::VectorXd;

constexpr double kBohrToAngstrom = 0.529177210903;  // CODATA 2018
constexpr double kAngstromToBohr = 1.0 / kBohrToAngstrom;
constexpr double kDebyePerAu = 2.541746473;          // 1 e*a0 in debye
constexpr double kPi = 3.14159265358979323846;

struct Error : std::runtime_error {
  using std::runtime_error::runtime_error;
};

}  // namespace qm
