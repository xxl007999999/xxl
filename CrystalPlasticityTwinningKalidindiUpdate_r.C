//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "CrystalPlasticityTwinningKalidindiUpdate_r.h"
#include "libmesh/int_range.h"

registerMooseObject("SolidMechanicsApp", CrystalPlasticityTwinningKalidindiUpdate_r);

InputParameters
CrystalPlasticityTwinningKalidindiUpdate_r::validParams()
{
  InputParameters params = CrystalPlasticityStressUpdateBase::validParams();
  params.addClassDescription(
      "Twinning propagation model based on Kalidindi's treatment of twinning in a FCC material");
  params.addParam<std::vector<unsigned int>>(
      "groups","To group the initial values on different ""twin systems 'format: [start end)', i.e.'0 ""4 8 11' groups 0-3, 4-7 and 8-11 ");
  params.addParam<Real>("r", 1.0, "Latent hardening coefficient");
  params.addParam<Real>(
      "initial_total_twin_volume_fraction",
      0.0,
      "The initial sum of the twin volume fraction across all twin systems in the crystal, if "
      "any. This value is distributed evenly across all twin systems in the crystal.");
  params.addRangeCheckedParam<Real>(
      "twin_reference_strain_rate",
      1.0e-3,
      "twin_reference_strain_rate>0",
      "The reference strain rate, gamma_o, for the power law plastic slip law "
      "due to twin propagation.");
  params.addRangeCheckedParam<Real>(
      "twin_strain_rate_sensitivity_exponent",
      0.05,
      "twin_strain_rate_sensitivity_exponent>0",
      "The strain rate sensitivity exponent for twin propagation power law "
      "strain rate calculation");
  params.addRangeCheckedParam<Real>(
      "characteristic_twin_shear",
      1.0 / std::sqrt(2.0),
      "characteristic_twin_shear>0",
      "The amount of shear that is associated with a twin in this cubic structure");
  params.addParam<std::vector<Real>>(
      "initial_twin_lattice_friction",
      "The initial value of the lattice friction for twin propogation, often "
      "calculated as a fraction of the Peierls strength");
  // params.addRangeCheckedParam<Real>(
  //     "non_coplanar_coefficient_twin_hardening",
  //     8000.0,
  //     "non_coplanar_coefficient_twin_hardening>=0",
  //     "The factor to apply to the hardening of non-coplanar twin systems "
  //     "strength as a function of the volume fraction of twins");
  // params.addRangeCheckedParam<Real>(
  //     "coplanar_coefficient_twin_hardening",
  //     800.0,
  //     "coplanar_coefficient_twin_hardening>=0",
  //     "Hardening coefficient for coplanar twin systems strength as a function of "
  //     "the volume fraction of twins");
  // params.addRangeCheckedParam<Real>(
  //     "non_coplanar_twin_hardening_exponent",
  //     0.05,
  //     "non_coplanar_twin_hardening_exponent>=0",
  //     "Parameter used to increase the hardening of non-coplanar twin systems such that the "
  //     "propagation of twins on conplanar systems is favored at early deformation stages.");
  params.addParam<std::vector<Real>>("t_sat", "saturated slip system strength");
  params.addParam<Real>("twin_h", "hardening constants");
  params.addRangeCheckedParam<Real>(
      "upper_limit_twin_volume_fraction",
      0.8,
      "upper_limit_twin_volume_fraction>0&upper_limit_twin_volume_fraction<=1",
      "The maximumum amount of twinning volume fraction allowed");


  return params;
}

CrystalPlasticityTwinningKalidindiUpdate_r::CrystalPlasticityTwinningKalidindiUpdate_r(
    const InputParameters & parameters)
  : CrystalPlasticityStressUpdateBase(parameters),
    _groups(getParam<std::vector<unsigned int>>("groups")),
    _total_twin_volume_fraction(declareProperty<Real>(_base_name + "total_volume_fraction_twins")),
    _total_twin_volume_fraction_old(
        getMaterialPropertyOld<Real>(_base_name + "total_volume_fraction_twins")),
    _initial_total_twin_volume_fraction(getParam<Real>("initial_total_twin_volume_fraction")),
    _twin_volume_fraction(
        declareProperty<std::vector<Real>>(_base_name + "twin_system_volume_fraction")),
    _twin_volume_fraction_old(
        getMaterialPropertyOld<std::vector<Real>>(_base_name + "twin_system_volume_fraction")),
    _twin_volume_fraction_increment(
        declareProperty<std::vector<Real>>(_base_name + "twin_system_volume_fraction_increment")),
    _reference_strain_rate(getParam<Real>("twin_reference_strain_rate")),
    _rate_sensitivity_exponent(getParam<Real>("twin_strain_rate_sensitivity_exponent")),
    _characteristic_twin_shear(getParam<Real>("characteristic_twin_shear")),
    _twin_initial_lattice_friction(getParam<std::vector<Real>>("initial_twin_lattice_friction")),
    // _non_coplanar_coefficient_twin_hardening(
    //     getParam<Real>("non_coplanar_coefficient_twin_hardening")),
    // _coplanar_coefficient_twin_hardening(getParam<Real>("coplanar_coefficient_twin_hardening")),
    // _noncoplanar_exponent(getParam<Real>("non_coplanar_twin_hardening_exponent")),
    _limit_twin_volume_fraction(getParam<Real>("upper_limit_twin_volume_fraction")),
    _r(getParam<Real>("r")),
    _tau_sat(getParam<std::vector<Real>>("t_sat")),
    _twin_h(getParam<Real>("twin_h")),
    _twin_initial_lattice_friction_per_slip(_number_slip_systems, 0.0),
    _tau_sat_per_slip(_number_slip_systems, 0.0),

    _acc_slip_per_sys(declareProperty<std::vector<Real>>(_base_name + "acc_slip")),
    _acc_slip_per_sys_old(getMaterialPropertyOld<std::vector<Real>>(_base_name + "acc_slip")),
    _accumulated_slip(declareProperty<Real>(_base_name + "accumulated_slip")),
    // resize local caching vectors used for substepping
    _previous_substep_acc_slip_per_sys(_number_slip_systems, 0.0),
    _acc_slip_per_sys_before_update(_number_slip_systems, 0.0),
    
    _hb(_number_slip_systems, 0.0),

    _twin_hardening_increment(_number_slip_systems, 0.0),
    _previous_substep_twin_resistance(_number_slip_systems, 0.0),
    _previous_substep_twin_volume_fraction(_number_slip_systems, 0.0),
    _twin_resistance_before_update(_number_slip_systems, 0.0),
    _twin_volume_fraction_before_update(_number_slip_systems, 0.0)
{
  getCrystalPlasticityParams(_tau_sat, _tau_sat_per_slip);
  getCrystalPlasticityParams(_twin_initial_lattice_friction, _twin_initial_lattice_friction_per_slip);
}

void
CrystalPlasticityTwinningKalidindiUpdate_r::getCrystalPlasticityParams(
    const std::vector<Real> & input_prop_vector, 
    std::vector<Real> & slip_prop_vector)
{
  unsigned int num_per_prop = 1;
  
  if (_groups.size() <= 0)
    mooseError("CrystalPlasticityAsaroUpdate: Error in reading initial state variable values: "
               "Specify input in .i file or in state_variable file");
  else if (_groups.size() != (input_prop_vector.size()/num_per_prop + 1))
    mooseError(
        "CrystalPlasticityAsaroUpdate: The size of the groups and slip_resistance_props does not match.");

  for (unsigned int i = 0; i < _groups.size() - 1; ++i)
  {
    unsigned int is, ie;

    is = _groups[i];
    ie = _groups[i + 1] - 1;

    if (is > ie)
      mooseError("CrystalPlasticityAsaroUpdate: Start index is = ",
                 is,
                 " should be greater than end index ie = ",
                 ie,
                 " in state variable read");

    for (unsigned int j = is; j <= ie; ++j)
      slip_prop_vector[j] = input_prop_vector[i*num_per_prop];
  }
  
  for (const auto i : make_range(_number_slip_systems))
  {
    if (!(slip_prop_vector[i] > 0.0))
    {
      mooseWarning(
          "CrystalPlasticityAsaroUpdate: Non-positive crystal plasticity model parameters ", slip_prop_vector[i]);
      break;
    }
  }
}

void
CrystalPlasticityTwinningKalidindiUpdate_r::initQpStatefulProperties()
{
  CrystalPlasticityStressUpdateBase::initQpStatefulProperties();

  // Resize constitutive-model specific material properties
  _twin_volume_fraction[_qp].resize(_number_slip_systems);
  _acc_slip_per_sys[_qp].resize(_number_slip_systems);

  // Set constitutive-model specific initial values from parameters
  _total_twin_volume_fraction[_qp] = _initial_total_twin_volume_fraction;
  const Real twin_volume_fraction_per_system =
      _initial_total_twin_volume_fraction / _number_slip_systems;
  for (const auto i : make_range(_number_slip_systems))
  {
    _twin_volume_fraction[_qp][i] = twin_volume_fraction_per_system;
    _slip_resistance[_qp][i] = _twin_initial_lattice_friction_per_slip[i];
    _slip_increment[_qp][i] = 0.0;
    _acc_slip_per_sys[_qp][i] = 0.0;
  }

  _accumulated_slip[_qp] = 0.0;
}

void
CrystalPlasticityTwinningKalidindiUpdate_r::setMaterialVectorSize()
{
  CrystalPlasticityStressUpdateBase::setMaterialVectorSize();

  // Resize non-stateful material properties
  _twin_volume_fraction_increment[_qp].resize(_number_slip_systems);
}

void
CrystalPlasticityTwinningKalidindiUpdate_r::setInitialConstitutiveVariableValues()
{
  _slip_resistance[_qp] = _slip_resistance_old[_qp];
  _previous_substep_twin_resistance = _slip_resistance_old[_qp];

  _twin_volume_fraction[_qp] = _twin_volume_fraction_old[_qp];
  _previous_substep_twin_volume_fraction = _twin_volume_fraction_old[_qp];

  _acc_slip_per_sys[_qp] = _acc_slip_per_sys_old[_qp];
  _previous_substep_acc_slip_per_sys = _acc_slip_per_sys_old[_qp];
}

void
CrystalPlasticityTwinningKalidindiUpdate_r::setSubstepConstitutiveVariableValues()
{
  _slip_resistance[_qp] = _previous_substep_twin_resistance;
  _twin_volume_fraction[_qp] = _previous_substep_twin_volume_fraction;

  _acc_slip_per_sys[_qp] = _previous_substep_acc_slip_per_sys;
}

bool
CrystalPlasticityTwinningKalidindiUpdate_r::calculateSlipRate()
{
  Real total_twin_volume_fraction = 0.0;
  for (const auto i : make_range(_number_slip_systems))
    total_twin_volume_fraction += _twin_volume_fraction[_qp][i];

  if (total_twin_volume_fraction < _limit_twin_volume_fraction)
  {
    for (const auto i : make_range(_number_slip_systems))
    {
      if (_tau[_qp][i] > 0.0)
      {
        const Real driving_force = std::abs(_tau[_qp][i] / _slip_resistance[_qp][i]);
        _slip_increment[_qp][i] = std::pow(driving_force, (1.0 / _rate_sensitivity_exponent)) *
                                  _reference_strain_rate ;
      }
      else // twin propagation is directional
        _slip_increment[_qp][i] = 0.0;

      // Check for allowable plastic strain due to twin propagation
      if (_slip_increment[_qp][i] * _substep_dt > _slip_incr_tol)
      {
        if (_print_convergence_message)
          mooseWarning("Maximum allowable plastic slip increment due to twinning exceeded the "
                       "user-defined tolerance on twin system ",
                       i,
                       ", with a value of",
                       _slip_increment[_qp][i] * _substep_dt,
                       " when the increment tolerance is set at ",
                       _slip_incr_tol);

        return false;
      }
    }
  }
  else // Once reach the limit of volume fraction, all subsequent increments will be zero
    std::fill(_slip_increment[_qp].begin(), _slip_increment[_qp].end(), 0.0);

  return true;
}

void
CrystalPlasticityTwinningKalidindiUpdate_r::calculateConstitutiveSlipDerivative(
    std::vector<Real> & dslip_dtau)
{
  Real total_twin_volume_fraction = 0.0;
  for (const auto i : make_range(_number_slip_systems))
    total_twin_volume_fraction += _twin_volume_fraction[_qp][i];

  // Once reach the limit of volume fraction, all plastic slip increments will be zero
  if (total_twin_volume_fraction >= _limit_twin_volume_fraction)
    std::fill(dslip_dtau.begin(), dslip_dtau.end(), 0.0);
  else
  {
    for (const auto i : make_range(_number_slip_systems))
    {
      if (_tau[_qp][i] <= 0.0)
        dslip_dtau[i] = 0.0;
      else
        // dslip_dtau[i] =
        //     _slip_increment[_qp][i] / (_rate_sensitivity_exponent * _tau[_qp][i]) * _substep_dt;
        dslip_dtau[i] =
            _slip_increment[_qp][i] / (_rate_sensitivity_exponent * _tau[_qp][i]) ;
    }
  }
}

bool
CrystalPlasticityTwinningKalidindiUpdate_r::areConstitutiveStateVariablesConverged()
{
  if (isConstitutiveStateVariableConverged(_twin_volume_fraction[_qp],
                                           _twin_volume_fraction_before_update,
                                           _previous_substep_twin_volume_fraction,
                                           _rel_state_var_tol) &&
      isConstitutiveStateVariableConverged(_slip_resistance[_qp],
                                           _twin_resistance_before_update,
                                           _previous_substep_twin_resistance,
                                           _resistance_tol))
    return true;
  return false;
}

void
CrystalPlasticityTwinningKalidindiUpdate_r::updateSubstepConstitutiveVariableValues()
{
  _previous_substep_twin_resistance = _slip_resistance[_qp];
  _previous_substep_twin_volume_fraction = _twin_volume_fraction[_qp];

  _previous_substep_acc_slip_per_sys = _acc_slip_per_sys[_qp];
}

void
CrystalPlasticityTwinningKalidindiUpdate_r::cacheStateVariablesBeforeUpdate()
{
  _twin_resistance_before_update = _slip_resistance[_qp];
  _twin_volume_fraction_before_update = _twin_volume_fraction[_qp];

  _acc_slip_per_sys_before_update = _acc_slip_per_sys[_qp];

}

void
CrystalPlasticityTwinningKalidindiUpdate_r::calculateStateVariableEvolutionRateComponent()
{
  // for (const auto i : make_range(_number_slip_systems))
  //   _twin_volume_fraction_increment[_qp][i] =
  //       _slip_increment[_qp][i] / _characteristic_twin_shear * _substep_dt;
  for (const auto i : make_range(_number_slip_systems))
    _twin_volume_fraction_increment[_qp][i] =
        _slip_increment[_qp][i] / _characteristic_twin_shear * _substep_dt;
}

bool
CrystalPlasticityTwinningKalidindiUpdate_r::updateStateVariables()
{
  if (calculateTwinVolumeFraction())
    return true;
  else
    return false;
}

bool
CrystalPlasticityTwinningKalidindiUpdate_r::calculateTwinVolumeFraction()
{
  _total_twin_volume_fraction[_qp] = 0.0;

  for (const auto i : make_range(_number_slip_systems))
  {
    if (_previous_substep_twin_volume_fraction[i] < _zero_tol &&
        _twin_volume_fraction_increment[_qp][i] < 0.0)
      _twin_volume_fraction[_qp][i] = _previous_substep_twin_volume_fraction[i];
    else
      _twin_volume_fraction[_qp][i] =
          _previous_substep_twin_volume_fraction[i] + _twin_volume_fraction_increment[_qp][i];

    if (_twin_volume_fraction[_qp][i] < 0.0)
    {
      if (_print_convergence_message)
        mooseWarning("A negative twin volume fraction value was computed: ",
                     _twin_volume_fraction[_qp][i],
                     " on twin system ",
                     i);
      return false;
    }
    else
      _total_twin_volume_fraction[_qp] += _twin_volume_fraction[_qp][i];
  }

  if ((_total_twin_volume_fraction[_qp] - _limit_twin_volume_fraction) >
      (_rel_state_var_tol * _limit_twin_volume_fraction))
  {
    if (_print_convergence_message)
      mooseWarning("Maximum allowable twin volume fraction limit exceeded with a value of ",
                   _total_twin_volume_fraction[_qp],
                   " when the limit is set as ",
                   _limit_twin_volume_fraction,
                   " with a user-set tolerance value of ",
                   _rel_state_var_tol);

    return false;
  }
  else
  {
    calculateTwinResistance();
    return true;
  }
}

void
CrystalPlasticityTwinningKalidindiUpdate_r::calculateTwinResistance()
{
  // DenseVector<Real> twin_hardening_increment(_number_slip_systems);

  for (const auto i : make_range(_number_slip_systems))
  {
    _twin_hardening_increment[i] = 0.0;

    Real val = std::cosh(_twin_h * _accumulated_slip[_qp] / (_tau_sat_per_slip[i] - _twin_initial_lattice_friction_per_slip[i])); // Karthik
    val = _twin_h * std::pow(1.0 / val, 2.0); // Kalidindi
    _hb[i] = val;

    // for (const auto j : make_range(_number_slip_systems))
    // {
    //   if (MooseUtils::relativeFuzzyEqual(_slip_plane_normal[j](0), _slip_plane_normal[i](0)) &&
    //       MooseUtils::relativeFuzzyEqual(_slip_plane_normal[j](1), _slip_plane_normal[i](1)))
    //   If the first two are the same, the third index will have to be as well
    //   {
    //     if (_slip_increment[_qp][j] > 0.0)
    //       twin_hardening_increment(i) += _coplanar_coefficient_twin_hardening *
    //                                      _total_twin_volume_fraction[_qp] *
    //                                      _twin_volume_fraction[_qp][j];
    //   }
    //   else // assume non-coplanar
    //   {
    //     if (_slip_increment[_qp][j] > 0.0)
    //       twin_hardening_increment(i) +=
    //           _non_coplanar_coefficient_twin_hardening *
    //           std::pow(_total_twin_volume_fraction[_qp], _noncoplanar_exponent) *
    //           _twin_volume_fraction[_qp][j];
    //   }
    // }
  }

  for (const auto i : make_range(_number_slip_systems))
  {
    for (const auto j : make_range(_number_slip_systems))
    {
      if (MooseUtils::relativeFuzzyEqual(_slip_plane_normal[j](0), _slip_plane_normal[i](0)) &&
          MooseUtils::relativeFuzzyEqual(_slip_plane_normal[j](1), _slip_plane_normal[i](1)))
      {  _twin_hardening_increment[i] +=
            std::abs(_slip_increment[_qp][j]) * _hb[j] * _substep_dt; // q_{ab} = 1.0 for self hardening
      }      
      else
      {  _twin_hardening_increment[i] +=
            std::abs(_slip_increment[_qp][j]) * _r * _hb[j] * _substep_dt; // latent hardenign
      }      
    }
  }

  for (const auto i : make_range(_number_slip_systems))
  {
    _twin_hardening_increment[i] *= _characteristic_twin_shear;
    if (_twin_hardening_increment[i] <= 0.0)
      _slip_resistance[_qp][i] = _previous_substep_twin_resistance[i];
    else
      _slip_resistance[_qp][i] = _twin_hardening_increment[i] + _previous_substep_twin_resistance[i];
  }

  for (const auto i : make_range(_number_slip_systems))
  {
    Real acc_slip_incr = 0.0;
    acc_slip_incr = std::abs(_slip_increment[_qp][i]) * _substep_dt;
    _acc_slip_per_sys[_qp][i] = _previous_substep_acc_slip_per_sys[i] + acc_slip_incr;
  }
  Real acc_slip_tmp = 0.0;
  for (const auto i : make_range(_number_slip_systems))
    acc_slip_tmp += _acc_slip_per_sys[_qp][i];

  _accumulated_slip[_qp] = acc_slip_tmp;

}
