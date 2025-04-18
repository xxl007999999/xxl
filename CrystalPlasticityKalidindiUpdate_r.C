//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "CrystalPlasticityKalidindiUpdate_r.h"
#include "libmesh/int_range.h"

registerMooseObject("SolidMechanicsApp", CrystalPlasticityKalidindiUpdate_r);

InputParameters
CrystalPlasticityKalidindiUpdate_r::validParams()
{
  InputParameters params = CrystalPlasticityStressUpdateBase::validParams();
  params.addClassDescription("Kalidindi version of homogeneous crystal plasticity.");
  params.addParam<std::vector<unsigned int>>("groups","To group the initial values on different ""slip systems 'format: [start end)', i.e.'0 ""4 8 11' groups 0-3, 4-7 and 8-11 ");

  params.addParam<Real>("r", 1.0, "Latent hardening coefficient");
  // params.addParam<Real>("h", 541.5, "hardening constants");
  // params.addParam<Real>("t_sat", 109.8, "saturated slip system strength");
  params.addParam<Real>("gss_a", 2.5, "coefficient for hardening");

  params.addParam<std::vector<Real>>("ao", "slip rate coefficient");
  params.addParam<std::vector<Real>>("h", "hardening constants");
  params.addParam<std::vector<Real>>("t_sat", "saturated slip system strength");
  params.addParam<std::vector<Real>>("xm", "exponent for slip rate");
  params.addParam<std::vector<Real>>("gss_initial", "initial lattice friction strength of the material");

  params.addParam<MaterialPropertyName>(
      "total_twin_volume_fraction",
      "Total twin volume fraction, if twinning is considered in the simulation");

  return params;
}

CrystalPlasticityKalidindiUpdate_r::CrystalPlasticityKalidindiUpdate_r(
    const InputParameters & parameters)
  : CrystalPlasticityStressUpdateBase(parameters),
    // Constitutive values
    _groups(getParam<std::vector<unsigned int>>("groups")),

    _r(getParam<Real>("r")),
    _gss_a(getParam<Real>("gss_a")),

    _ao(getParam<std::vector<Real>>("ao")),
    _h(getParam<std::vector<Real>>("h")),
    _tau_sat(getParam<std::vector<Real>>("t_sat")),
    _xm(getParam<std::vector<Real>>("xm")),
    _gss_initial(getParam<std::vector<Real>>("gss_initial")),
    _ao_per_slip(_number_slip_systems, 0.0),
    _xm_per_slip(_number_slip_systems, 0.0),
    _gss_initial_per_slip(_number_slip_systems, 0.0),
    _h_per_slip(_number_slip_systems, 0.0),
    _tau_sat_per_slip(_number_slip_systems, 0.0),

    // resize vectors used in the consititutive slip hardening
    _hb(_number_slip_systems, 0.0),
    _slip_resistance_increment(_number_slip_systems, 0.0),

    _acc_slip_per_sys(declareProperty<std::vector<Real>>(_base_name + "acc_slip")),
    _acc_slip_per_sys_old(getMaterialPropertyOld<std::vector<Real>>(_base_name + "acc_slip")),
    _accumulated_slip(declareProperty<Real>(_base_name + "accumulated_slip")),

    // resize local caching vectors used for substepping
    _previous_substep_slip_resistance(_number_slip_systems, 0.0),
    _slip_resistance_before_update(_number_slip_systems, 0.0),
    
    _previous_substep_acc_slip_per_sys(_number_slip_systems, 0.0),
    _acc_slip_per_sys_before_update(_number_slip_systems, 0.0),
    // Twinning contributions, if used
    _include_twinning_in_Lp(parameters.isParamValid("total_twin_volume_fraction")),
    _twin_volume_fraction_total(_include_twinning_in_Lp
                                    ? &getMaterialPropertyOld<Real>("total_twin_volume_fraction")
                                    : nullptr)
{
  getCrystalPlasticityParams(_ao, _ao_per_slip);
  getCrystalPlasticityParams(_xm, _xm_per_slip);
  getCrystalPlasticityParams(_gss_initial, _gss_initial_per_slip);
  getCrystalPlasticityParams(_h, _h_per_slip);
  getCrystalPlasticityParams(_tau_sat, _tau_sat_per_slip);
}

void
CrystalPlasticityKalidindiUpdate_r::getCrystalPlasticityParams(
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
CrystalPlasticityKalidindiUpdate_r::initQpStatefulProperties()
{
  CrystalPlasticityStressUpdateBase::initQpStatefulProperties();

  _acc_slip_per_sys[_qp].resize(_number_slip_systems);

  for (const auto i : make_range(_number_slip_systems))
  {
    _slip_resistance[_qp][i] = _gss_initial_per_slip[i];
    _slip_increment[_qp][i] = 0.0;
    _acc_slip_per_sys[_qp][i] = 0.0;
  }

  _accumulated_slip[_qp] = 0.0;
}

void
CrystalPlasticityKalidindiUpdate_r::setInitialConstitutiveVariableValues()
{
  // Would also set old dislocation densities here if included in this model
  _slip_resistance[_qp] = _slip_resistance_old[_qp];
  _previous_substep_slip_resistance = _slip_resistance_old[_qp];

  _acc_slip_per_sys[_qp] = _acc_slip_per_sys_old[_qp];
  _previous_substep_acc_slip_per_sys = _acc_slip_per_sys_old[_qp];
}

void
CrystalPlasticityKalidindiUpdate_r::setSubstepConstitutiveVariableValues()
{
  // Would also set substepped dislocation densities here if included in this model
  _slip_resistance[_qp] = _previous_substep_slip_resistance;

  _acc_slip_per_sys[_qp] = _previous_substep_acc_slip_per_sys;
}

bool
CrystalPlasticityKalidindiUpdate_r::calculateSlipRate()
{
  for (const auto i : make_range(_number_slip_systems))
  {
    // _slip_increment[_qp][i] =
    //     _ao_per_slip[i] * std::pow(std::abs(_tau[_qp][i] / _slip_resistance[_qp][i]), 1.0 / _xm_per_slip[i]);
    _slip_increment[_qp][i] =
        _ao_per_slip[i] * std::pow(std::abs(_tau[_qp][i] / _slip_resistance[_qp][i]), 1.0 / _xm_per_slip[i]) ;

    if (_tau[_qp][i] < 0.0)
      _slip_increment[_qp][i] *= -1.0;

    if (std::abs(_slip_increment[_qp][i]) * _substep_dt > _slip_incr_tol)
    {
      if (_print_convergence_message)
        mooseWarning("Maximum allowable slip increment exceeded ",
                     std::abs(_slip_increment[_qp][i]) * _substep_dt);

      return false;
    }
  }
  return true;
}

void
CrystalPlasticityKalidindiUpdate_r::calculateEquivalentSlipIncrement(
    RankTwoTensor & equivalent_slip_increment)
{
  if (_include_twinning_in_Lp)
  {
    for (const auto i : make_range(_number_slip_systems))
      equivalent_slip_increment += (1.0 - (*_twin_volume_fraction_total)[_qp]) *
                                   _flow_direction[_qp][i] * _slip_increment[_qp][i] * _substep_dt;
  }
  else // if no twinning volume fraction material property supplied, use base class
    CrystalPlasticityStressUpdateBase::calculateEquivalentSlipIncrement(equivalent_slip_increment);
}

void
CrystalPlasticityKalidindiUpdate_r::calculateConstitutiveSlipDerivative(
    std::vector<Real> & dslip_dtau)
{
  for (const auto i : make_range(_number_slip_systems))
  {
    if (MooseUtils::absoluteFuzzyEqual(_tau[_qp][i], 0.0))
      dslip_dtau[i] = 0.0;
    else
      dslip_dtau[i] = _ao_per_slip[i] / _xm_per_slip[i] *
                      std::pow(std::abs(_tau[_qp][i] / _slip_resistance[_qp][i]), 1.0 / _xm_per_slip[i] - 1.0) /
                      _slip_resistance[_qp][i];
  }
}

bool
CrystalPlasticityKalidindiUpdate_r::areConstitutiveStateVariablesConverged()
{
  return isConstitutiveStateVariableConverged(_slip_resistance[_qp],
                                              _slip_resistance_before_update,
                                              _previous_substep_slip_resistance,
                                              _resistance_tol);
}

void
CrystalPlasticityKalidindiUpdate_r::updateSubstepConstitutiveVariableValues()
{
  // Would also set substepped dislocation densities here if included in this model
  _previous_substep_slip_resistance = _slip_resistance[_qp];

  _previous_substep_acc_slip_per_sys = _acc_slip_per_sys[_qp];

}

void
CrystalPlasticityKalidindiUpdate_r::cacheStateVariablesBeforeUpdate()
{
  _slip_resistance_before_update = _slip_resistance[_qp];

  _acc_slip_per_sys_before_update = _acc_slip_per_sys[_qp];

}

void
CrystalPlasticityKalidindiUpdate_r::calculateStateVariableEvolutionRateComponent()
{
  for (const auto i : make_range(_number_slip_systems))
  {
    // Clear out increment from the previous iteration
    // _slip_resistance_increment[i] = 0.0;

    //     _hb[i] = _h_per_slip[i] * std::pow(std::abs(1.0 - _slip_resistance[_qp][i] / _tau_sat_per_slip[i]), _gss_a);
    // const Real hsign = 1.0 - _slip_resistance[_qp][i] / _tau_sat_per_slip[i];
    // if (hsign < 0.0)
    //   _hb[i] *= -1.0;
    _slip_resistance_increment[i] = 0.0;
    
    Real val = std::cosh(_h_per_slip[i] * _accumulated_slip[_qp] / (_tau_sat_per_slip[i] - _gss_initial_per_slip[i])); // Karthik
    val = _h_per_slip[i] * std::pow(1.0 / val, 2.0); // Kalidindi
    
    _hb[i] = val;
  }

  for (const auto i : make_range(_number_slip_systems))
  {
    for (const auto j : make_range(_number_slip_systems))
    {
      unsigned int iplane, jplane;
      iplane = i / 3;
      jplane = j / 3;

      if (iplane == jplane) // self vs. latent hardening
        _slip_resistance_increment[i] +=
            std::abs(_slip_increment[_qp][j]) * _hb[j]; // q_{ab} = 1.0 for self hardening
      else
        _slip_resistance_increment[i] +=
            std::abs(_slip_increment[_qp][j]) * _r * _hb[j]; // latent hardenign
    }
  }
}

bool
CrystalPlasticityKalidindiUpdate_r::updateStateVariables()
{
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
  // Now perform the check to see if the slip system should be updated
  for (const auto i : make_range(_number_slip_systems))
  {
    _slip_resistance_increment[i] *= _substep_dt;
    if (_previous_substep_slip_resistance[i] < _zero_tol && _slip_resistance_increment[i] < 0.0)
      _slip_resistance[_qp][i] = _previous_substep_slip_resistance[i];
    else
      _slip_resistance[_qp][i] =
          _previous_substep_slip_resistance[i] + _slip_resistance_increment[i];

    if (_slip_resistance[_qp][i] < 0.0)
      return false;
  }
  return true;
}
