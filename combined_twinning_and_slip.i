[GlobalParams]
  displacements = 'ux uy'
[]

[Mesh]
  [NeperMesh]
    type = FileMeshGenerator
    file = lsp2d_fz.msh
  []
  [x0_modifier]  #left
    type = BoundingBoxNodeSetGenerator
    input = NeperMesh
    new_boundary = x0
    top_right = '0 0.1 0'
    bottom_left = '0 0 0'
  []
  [x1_modifier]  #right
    type = BoundingBoxNodeSetGenerator
    input = x0_modifier
    new_boundary = x1
    top_right = '0.1 0.1 0'
    bottom_left = '0.1 0 0'
  []
  [y0_modifier]  # bottom
    type = BoundingBoxNodeSetGenerator
    input = x1_modifier
    new_boundary = y0
    top_right = '0.1 0 0'
    bottom_left = '0 0 0'
  []
  [y1_modifier]  # top
    type = BoundingBoxNodeSetGenerator
    input = y0_modifier
    new_boundary = y1
    top_right = '0.1 0.1 0'
    bottom_left = '0 0.1 0'
  []
  construct_side_list_from_node_list = true
[]

[AuxVariables]
  [pk2]
    order = CONSTANT
    family = MONOMIAL
  []
  [stress_theta]
    order = CONSTANT
    family = MONOMIAL
  []
  [strain_theta]
    order = CONSTANT
    family = MONOMIAL
  []
  [e_zz]
    order = CONSTANT
    family = MONOMIAL
  []
  [fp_zz]
    order = CONSTANT
    family = MONOMIAL
  []
  [total_twin_volume_fraction_gamma]
    order = CONSTANT
    family = MONOMIAL
  []
  # [total_twin_volume_fraction_alpha]
  #   order = CONSTANT
  #   family = MONOMIAL
  # []
  [accumulated_gamma_twin]
    order = CONSTANT
    family = MONOMIAL
  []
  [accumulated_gamma_slip]
    order = CONSTANT
    family = MONOMIAL
  []
  [accumulated_alpha_slip]
    order = CONSTANT
    family = MONOMIAL
  []
  # [twin_volume_fraction_3]
  #   order = CONSTANT
  #   family = MONOMIAL
  # []
  [gss]
    order = CONSTANT
    family = MONOMIAL
  []
[]

[Physics/SolidMechanics/QuasiStatic/all]
  strain = FINITE
  add_variables = true
  generate_output = 'stress_yy strain_yy'
  use_finite_deform_jacobian = true
  material_output_order = FIRST
  material_output_family = LAGRANGE
[]

[AuxKernels]
  [pk2]
    type = RankTwoAux
    variable = pk2
    rank_two_tensor = second_piola_kirchhoff_stress
    index_j = 1
    index_i = 1
    execute_on = timestep_end
  []
  [stress_theta]
    type = RankTwoAux
    rank_two_tensor = stress
    index_i = 1
    index_j = 1
    variable = stress_theta
    execute_on = timestep_end
  []
  [strain_theta]
    type = RankTwoAux
    rank_two_tensor = total_strain
    index_i = 1
    index_j = 1
    variable = strain_theta
    execute_on = timestep_end
  []
  [e_zz]
    type = RankTwoAux
    variable = e_zz
    rank_two_tensor = total_lagrangian_strain
    index_j = 1
    index_i = 1
    execute_on = timestep_end
  []
  [fp_zz]
    type = RankTwoAux
    variable = fp_zz
    rank_two_tensor = plastic_deformation_gradient
    index_j = 1
    index_i = 1
    execute_on = timestep_end
  []
  [total_twin_volume_fraction_gamma]
    type = MaterialRealAux
    variable = total_twin_volume_fraction_gamma
    property = twin_total_volume_fraction_twins
    execute_on = timestep_end
    block = '1 2 3 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 29 30 31 32 33 34 36 37 38 39 40 41 42 43 44 45 46 47 48 49 50 51 52 54 55 57 58 59 60 61 62 63 64 65 66 67 68 70 71 72 73 74 76 77 78 80 81 82 83 84 86 87 88 90 91 92 93 94 95 96 97 98 99 101 102 103 104 105 106 107'
  []
  # [total_twin_volume_fraction_alpha]
  #   type = MaterialRealAux
  #   variable = total_twin_volume_fraction_alpha
  #   property = twin_total_volume_fraction_twins
  #   execute_on = timestep_end
  #   block = '1 2 3 5 6 7 10 11 13 14 15 16 19 21 22 24 25 26 27 31 33 34 35 36 37 38 40 41 42 43 44 45 46 47 48 49 50'
  # []
  [accumulated_gamma_twin]
    type = MaterialRealAux
    variable = accumulated_gamma_twin
    property = twin_accumulated_slip
    execute_on = timestep_end
    block = '1 2 3 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 29 30 31 32 33 34 36 37 38 39 40 41 42 43 44 45 46 47 48 49 50 51 52 54 55 57 58 59 60 61 62 63 64 65 66 67 68 70 71 72 73 74 76 77 78 80 81 82 83 84 86 87 88 90 91 92 93 94 95 96 97 98 99 101 102 103 104 105 106 107'
  []
  [accumulated_gamma_slip]
    type = MaterialRealAux
    variable = accumulated_gamma_slip
    property = accumulated_slip
    execute_on = timestep_end
    block = '1 2 3 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 29 30 31 32 33 34 36 37 38 39 40 41 42 43 44 45 46 47 48 49 50 51 52 54 55 57 58 59 60 61 62 63 64 65 66 67 68 70 71 72 73 74 76 77 78 80 81 82 83 84 86 87 88 90 91 92 93 94 95 96 97 98 99 101 102 103 104 105 106 107'
  []
  [accumulated_alpha_slip]
    type = MaterialRealAux
    variable = accumulated_alpha_slip
    property = accumulated_slip
    execute_on = timestep_end
    block = '11 14 18 22 34 39 55 57 59 61 66 68 86 88 98 103'
  []
  # [slip_increment_3]
  #   type = MaterialStdVectorAux
  #   variable = slip_increment_3
  #   property = slip_increment
  #   index = 3
  #   execute_on = timestep_end
  # []
  # [slip_increment_4]
  #   type = MaterialStdVectorAux
  #   variable = slip_increment_4
  #   property = slip_increment
  #   index = 4
  #   execute_on = timestep_end
  # []
  # [slip_increment_5]
  #   type = MaterialStdVectorAux
  #   variable = slip_increment_5
  #   property = slip_increment
  #   index = 5
  #   execute_on = timestep_end
  # []
  # [slip_increment_6]
  #   type = MaterialStdVectorAux
  #   variable = slip_increment_6
  #   property = slip_increment
  #   index = 6
  #   execute_on = timestep_end
  # []
  # [slip_increment_7]
  #   type = MaterialStdVectorAux
  #   variable = slip_increment_7
  #   property = slip_increment
  #   index = 7
  #   execute_on = timestep_end
  # []
  # [slip_increment_8]
  #   type = MaterialStdVectorAux
  #   variable = slip_increment_8
  #   property = slip_increment
  #   index = 8
  #   execute_on = timestep_end
  # []
  # [slip_increment_9]
  #   type = MaterialStdVectorAux
  #   variable = slip_increment_9
  #   property = slip_increment
  #   index = 9
  #   execute_on = timestep_end
  # []
  # [slip_increment_10]
  #   type = MaterialStdVectorAux
  #   variable = slip_increment_10
  #   property = slip_increment
  #   index = 10
  #   execute_on = timestep_end
  # []
  # [slip_increment_11]
  #   type = MaterialStdVectorAux
  #   variable = slip_increment_11
  #   property = slip_increment
  #   index = 11
  #   execute_on = timestep_end
  # []
  # [twin_volume_fraction_0]
  #   type = MaterialStdVectorAux
  #   variable = twin_volume_fraction_0
  #   property = twin_twin_system_volume_fraction
  #   index = 0
  #   execute_on = timestep_end
  # []
  # [twin_volume_fraction_1]
  #   type = MaterialStdVectorAux
  #   variable = twin_volume_fraction_1
  #   property = twin_twin_system_volume_fraction
  #   index = 1
  #   execute_on = timestep_end
  # []
  # [twin_volume_fraction_2]
  #   type = MaterialStdVectorAux
  #   variable = twin_volume_fraction_2
  #   property = twin_twin_system_volume_fraction
  #   index = 2
  #   execute_on = timestep_end
  # []
  # [twin_volume_fraction_3]
  #   type = MaterialStdVectorAux
  #   variable = twin_volume_fraction_3
  #   property = twin_twin_system_volume_fraction
  #   index = 3
  #   execute_on = timestep_end
  # []
  [gss]
    type = MaterialStdVectorAux
    variable = gss
    property = slip_resistance
    index = 0
    execute_on = timestep_end
  []
[]

[BCs]
  [x0_left]
    type = DirichletBC
    variable = ux
    boundary = x0
    #boundary = bottom
    value = 0.0
  []
  [y0_bottom]
    type = DirichletBC
    variable = uy
    boundary = y0
    #boundary = left
    value = 0.0
  []
  [pully]
    type = FunctionDirichletBC
    variable = uy
    boundary = y1
    function = '4.0e-4 * t'
  []
[]

[UserObjects]
  [prop_read]
    type = PropertyReadFile
    prop_file_name = 'lsp2d_fz.ori'
    nprop = 3
    nblock = 107
    read_type = block
    use_zero_based_block_indexing = false
  []
  [diameter]
    type = PropertyReadFile
    prop_file_name = 'lsp2d_fz.stcell'
    nprop = 1
    nblock = 107
    read_type = block
    use_zero_based_block_indexing = false
  []
[]

[Materials]
  [elasticity_tensor_11]
    type = ComputeElasticityTensorCP_r
    C_ijkl = '1.88e5 1.01e5 0.99e5 1.88e5 0.99e5 1.79e5 1.19e5 1.19e5 0.75e5'  # roughly copper
    fill_method = axisymmetric_rz
    euler_angle_1 = 45.0
    euler_angle_2 = 54.7
    euler_angle_3 = 30.0
    read_prop_user_object = prop_read
    output_properties = 'Euler_angles'
    block = '11 14 18 22 34 39 55 57 59 61 66 68 86 88 98 103'
  []
  [elasticity_tensor_12]
    type = ComputeElasticityTensorCP_r
    C_ijkl = '1.88e5 1.01e5 0.99e5 1.88e5 0.99e5 1.79e5 1.19e5 1.19e5 0.75e5'  # roughly copper
    fill_method = axisymmetric_rz
    euler_angle_1 = 45.0
    euler_angle_2 = 54.7
    euler_angle_3 = 90.0
    read_prop_user_object = prop_read
    output_properties = 'Euler_angles'
    block = '7 9 15 17 25 30 40 45 48 54 70 73 76 84 91 95'
  []
  [elasticity_tensor_13]
    type = ComputeElasticityTensorCP_r
    C_ijkl = '1.88e5 1.01e5 0.99e5 1.88e5 0.99e5 1.79e5 1.19e5 1.19e5 0.75e5'  # roughly copper
    fill_method = axisymmetric_rz
    euler_angle_1 = 45.0
    euler_angle_2 = 54.7
    euler_angle_3 = 150.0
    read_prop_user_object = prop_read
    output_properties = 'Euler_angles'
    block = '3 5 36 38 42 51 58 64 72 74 78 81 83 90 94 105'
  []
  [elasticity_tensor_14]
    type = ComputeElasticityTensorCP_r
    C_ijkl = '1.88e5 1.01e5 0.99e5 1.88e5 0.99e5 1.79e5 1.19e5 1.19e5 0.75e5'  # roughly copper
    fill_method = axisymmetric_rz
    euler_angle_1 = 45.0
    euler_angle_2 = 54.7
    euler_angle_3 = 210.0
    read_prop_user_object = prop_read
    output_properties = 'Euler_angles'
    block = '2 8 20 24 32 41 44 46 52 62 71 87 96 99 102 107'
  []
  [elasticity_tensor_15]
    type = ComputeElasticityTensorCP_r
    C_ijkl = '1.88e5 1.01e5 0.99e5 1.88e5 0.99e5 1.79e5 1.19e5 1.19e5 0.75e5'  # roughly copper
    fill_method = axisymmetric_rz
    euler_angle_1 = 45.0
    euler_angle_2 = 54.7
    euler_angle_3 = 270.0
    read_prop_user_object = prop_read
    output_properties = 'Euler_angles'
    block = '6 12 16 19 26 31 33 37 49 65 67 80 93 97 101 106'
  []
  [elasticity_tensor_16]
    type = ComputeElasticityTensorCP_r
    C_ijkl = '1.88e5 1.01e5 0.99e5 1.88e5 0.99e5 1.79e5 1.19e5 1.19e5 0.75e5'  # roughly copper
    fill_method = axisymmetric_rz
    euler_angle_1 = 45.0
    euler_angle_2 = 54.7
    euler_angle_3 = 330.0
    read_prop_user_object = prop_read
    output_properties = 'Euler_angles'
    block = '1 10 13 21 23 27 29 43 47 50 60 63 77 82 92 104'
  []
  [elasticity_tensor_2]
    type = ComputeElasticityTensorCP_r
    C_ijkl = '2.21e5 0.71e5 0.85e5 2.38e5 0.69e5'  #c11 c12 c13 c33 c44 {hcp Ti-64}
    fill_method = axisymmetric_rz
    read_prop_user_object = prop_read
    output_properties = 'Euler_angles'
    block = '4 28 35 53 56 69 75 79 85 89 100'
  []
  [stress_1]
    type = ComputeMultipleCrystalPlasticityStress
    crystal_plasticity_models = 'twin_xtalpl_1 slip_xtalpl_1'
    tan_mod_type = exact
    maximum_substep_iteration = 10
    block = '1 2 3 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 29 30 31 32 33 34 36 37 38 39 40 41 42 43 44 45 46 47 48 49 50 51 52 54 55 57 58 59 60 61 62 63 64 65 66 67 68 70 71 72 73 74 76 77 78 80 81 82 83 84 86 87 88 90 91 92 93 94 95 96 97 98 99 101 102 103 104 105 106 107'
  []
  [stress_2]
    type = ComputeMultipleCrystalPlasticityStress
    crystal_plasticity_models = 'slip_xtalpl_2'
    tan_mod_type = exact
    maximum_substep_iteration = 10
    block = '4 28 35 53 56 69 75 79 85 89 100'
  []
  [twin_xtalpl_1]
    type = CrystalPlasticityTwinningKalidindiUpdate_r
    base_name = twin
    number_slip_systems = 4
    slip_sys_file_name = fcc_input_twinning_systems.txt
    # crystal_lattice_type = FCC
    # unit_cell_dimension = '3.98e-7 3.98e-7 4.05e-7'
    # "normal given before the slip plane direction."
    # characteristic_twin_shear = 0.07
    initial_total_twin_volume_fraction = 0.0
    twin_reference_strain_rate = 0.0014   #2.36e-4
    groups = '0 1 4'
    twin_strain_rate_sensitivity_exponent = 0.1  #0.05
    initial_twin_lattice_friction = '70.0 252.0' #67.2
    t_sat = '142.0 493.6' #87.2
    upper_limit_twin_volume_fraction = 0.9
    r = 1.0
    twin_h = 371.0
    block = '1 2 3 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 29 30 31 32 33 34 36 37 38 39 40 41 42 43 44 45 46 47 48 49 50 51 52 54 55 57 58 59 60 61 62 63 64 65 66 67 68 70 71 72 73 74 76 77 78 80 81 82 83 84 86 87 88 90 91 92 93 94 95 96 97 98 99 101 102 103 104 105 106 107'
  []
  [slip_xtalpl_1]
    type = CrystalPlasticityKalidindiUpdate_r
    number_slip_systems = 12
    slip_sys_file_name = fcc_input_slip_sys.txt
    total_twin_volume_fraction = twin_total_volume_fraction_twins
    crystal_lattice_type = FCC
    unit_cell_dimension = '3.98e-7 3.98e-7 4.05e-7'
    groups = '0 1 3 4 6 7 8 9 12'
    gss_initial = '90.0 302.0 244.0 162.0 439.2 543.6 439.2 543.6' #88.6
    ao = '0.001 0.001 0.001 0.001 0.001 0.001 0.001 0.001'          #'0.001 0.001'
    xm = '0.1 0.1 0.1 0.1 0.1 0.1 0.1 0.1'     #'0.05 0.05'
    h = '1100.0 371.0 880.0 1100.0 880.0 371.0 880.0 371.0'
    t_sat = '162.0 543.6 439.2 291.6 790.2 977.4 790.2 977.4' #115.2
    r = 1.0
    block = '1 2 3 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 29 30 31 32 33 34 36 37 38 39 40 41 42 43 44 45 46 47 48 49 50 51 52 54 55 57 58 59 60 61 62 63 64 65 66 67 68 70 71 72 73 74 76 77 78 80 81 82 83 84 86 87 88 90 91 92 93 94 95 96 97 98 99 101 102 103 104 105 106 107'
  []
  # [twin_xtalpl_2]
  #   type = CrystalPlasticityTwinningKalidindiUpdate_r
  #   base_name = twin
  #   number_slip_systems = 1
  #   slip_sys_file_name = hcp_input_twinning_systems.txt
  #   crystal_lattice_type = HCP
  #   unit_cell_dimension = '5.77e-7 5.77e-7 4.616e-7'
  #   #"normal given before the slip plane direction."
  #   initial_total_twin_volume_fraction = 0.0
  #   twin_reference_strain_rate = 1.0e-7    #0.001
  #   twin_strain_rate_sensitivity_exponent = 0.05  #0.05
  #   initial_twin_lattice_friction = 130.0
  #   t_sat = 169.0
  #   upper_limit_twin_volume_fraction = 0.9
  #   r = 1.0
  #   twin_h = 400.0
  #   block = '1 2 3 5 6 7 10 11 13 14 15 16 19 21 22 24 25 26 27 31 33 34 35 36 37 38 40 41 42 43 44 45 46 47 48 49 50'  #重新修改材料参数
  # []
  [slip_xtalpl_2]
    type = CrystalPlasticityKalidindiUpdate_r
    number_slip_systems = 12
    slip_sys_file_name = hcp_input_slip_sys.txt
    # total_twin_volume_fraction = twin_total_volume_fraction_twins
    crystal_lattice_type = HCP
    unit_cell_dimension = '5.77e-7 5.77e-7 4.616e-7'
    groups = '0 3 6 12'
    gss_initial = '330.0 100.0 910.0' #227.8
    ao = '0.001 0.001 0.001'
    xm = '0.1 0.1 0.1'
    h = '96.0 329.0 911.0'
    t_sat = '594.0 180.0 1638.0' #296.2
    r = 1.0
    block = '4 28 35 53 56 69 75 79 85 89 100'
  []
[]

[Postprocessors]
  [fp_zz]
    type = ElementAverageValue
    variable = fp_zz
  []
  [stress_theta]
    type = ElementAverageValue
    variable = stress_theta
  []
  [strain_theta]
    type = ElementAverageValue
    variable = strain_theta
  []
  [e_zz]
    type = ElementAverageValue
    variable = e_zz
  []
  [pk2]
    type = ElementAverageValue
    variable = pk2
  []
  [total_twin_volume_fraction_gamma]
    type = ElementAverageValue
    variable = total_twin_volume_fraction_gamma
    #block = '4 8 9 12 17 18 20 23 28 29 30 32 39'
  []
  # [total_twin_volume_fraction_alpha]
  #   type = ElementAverageValue
  #   variable = total_twin_volume_fraction_alpha
  #   #block = '1 2 3 5 6 7 10 11 13 14 15 16 19 21 22 24 25 26 27 31 33 34 35 36 37 38 40 41 42 43 44 45 46 47 48 49 50'
  # []
  [gss]
    type = ElementAverageValue
    variable = gss
  []
  [accumulated_gamma_twin]
    type = ElementAverageValue
    variable = accumulated_gamma_twin
  []
  [accumulated_gamma_slip]
    type = ElementAverageValue
    variable = accumulated_gamma_slip
  []
  [accumulated_alpha_slip]
    type = ElementAverageValue
    variable = accumulated_alpha_slip
  []
  # [twin_volume_fraction_3]
  #   type = ElementAverageValue
  #   variable = twin_volume_fraction_3
  # []
  [stress_yy]
    type = ElementAverageValue
    variable = stress_yy
  []
  [uy_avg_front]
    type = SideAverageValue
    variable = uy
    boundary = y1
  []
  [strain_yy]
    type = ElementAverageValue
    variable = strain_yy
  []
[]

[Preconditioning]
  [smp]
    type = SMP
    full = true
  []
[]

[Executioner]
  type = Transient
  solve_type = 'NEWTON'

  petsc_options_iname = '-pc_type -pc_asm_overlap -sub_pc_type -ksp_type -ksp_gmres_restart'
  petsc_options_value = ' asm      2              lu            gmres     200'
  #petsc_options_iname = '-pc_type -pc_factor_mat_solver_package'
  #petsc_options_value = ' lu       superlu_dist'
  
  nl_abs_tol = 1e-6
  nl_rel_tol = 1e-5
  nl_abs_step_tol = 1e-6
  nl_rel_step_tol = 1e-5

  dt = 1e-2
  #dtmin = 1e-6
  #num_steps = 2
  end_time = 10
[]

[Outputs]
  print_linear_residuals = false
  time_step_interval = 10
  csv = true
  exodus = true
[]

[Debug]
  show_var_residual_norms = false
[]

