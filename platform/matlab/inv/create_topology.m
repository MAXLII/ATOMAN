function create_topology()
%CREATE_TOPOLOGY Create the inverter Simulink power-stage model.
%
% Run this script from MATLAB:
%   cd <repo_root>/matlab/inv
%   create_topology

model = 'inv_mod';
ctrlTs = '3.3333333703922108e-5';
halfCtrlTs = '1.6666666851961054e-5';
pwmTickTs = '1.6666666851961055e-7';

if bdIsLoaded(model)
    set_param(model, 'Dirty', 'off');
    close_system(model, 0);
end

new_system(model);
open_system(model);

set_param(model, ...
    'StopTime', '0.35', ...
    'SolverType', 'Fixed-step', ...
    'Solver', 'ode3', ...
    'FixedStep', pwmTickTs, ...
    'ReturnWorkspaceOutputs', 'off');

add_block('spspowerguiLib/powergui', [model '/powergui'], ...
    'Position', [40 30 125 70]);
set_param([model '/powergui'], 'SimulationMode', 'Discrete', 'SampleTime', pwmTickTs);

add_block('powerlib/Electrical Sources/DC Voltage Source', [model '/Vdc'], ...
    'Position', [80 260 140 340]);
set_param([model '/Vdc'], 'Amplitude', '400');

add_block('powerlib/Elements/Ground', [model '/Ground'], ...
    'Position', [85 410 135 460]);

add_block('powerlib/Power Electronics/IGBT//Diode', [model '/Q1_Fast_Upper'], ...
    'Position', [320 150 380 230]);
add_block('powerlib/Power Electronics/IGBT//Diode', [model '/Q2_Fast_Lower'], ...
    'Position', [320 350 380 430]);
add_block('powerlib/Power Electronics/IGBT//Diode', [model '/Q3_Slow_Upper'], ...
    'Position', [560 150 620 230]);
add_block('powerlib/Power Electronics/IGBT//Diode', [model '/Q4_Slow_Lower'], ...
    'Position', [560 350 620 430]);

switches = {'Q1_Fast_Upper', 'Q2_Fast_Lower', 'Q3_Slow_Upper', 'Q4_Slow_Lower'};
for k = 1:numel(switches)
    set_param([model '/' switches{k}], ...
        'Ron', '1e-3', ...
        'Rs', '1e5', ...
        'Cs', 'inf');
end

add_block('powerlib/Measurements/Current Measurement', [model '/I_L_Meas'], ...
    'Position', [720 248 770 302]);

add_block('powerlib/Elements/Series RLC Branch', [model '/L_Filter'], ...
    'Position', [825 250 905 300]);
set_param([model '/L_Filter'], ...
    'BranchType', 'L', ...
    'Resistance', '0.05', ...
    'Inductance', '440e-6');

add_block('powerlib/Elements/Series RLC Branch', [model '/C_Filter'], ...
    'Position', [1010 310 1060 390]);
set_param([model '/C_Filter'], ...
    'BranchType', 'C', ...
    'Capacitance', '12e-6');

add_block('powerlib/Elements/Series RLC Branch', [model '/R_Load'], ...
    'Position', [1120 310 1170 390]);
set_param([model '/R_Load'], ...
    'BranchType', 'R', ...
    'Resistance', '100');

add_block('powerlib/Measurements/Voltage Measurement', [model '/V_Cap_Meas'], ...
    'Position', [1000 180 1060 230]);
add_block('powerlib/Measurements/Voltage Measurement', [model '/V_Bus_Meas'], ...
    'Position', [170 250 230 305]);

add_block('simulink/User-Defined Functions/S-Function', [model '/sfunc'], ...
    'Position', [285 610 365 665], ...
    'FunctionName', 'sfunc');

add_block('simulink/Signal Routing/Mux', [model '/Input_Mux'], ...
    'Position', [205 610 225 700], ...
    'Inputs', '4');
add_block('simulink/Signal Routing/Demux', [model '/Output_Demux'], ...
    'Position', [430 590 455 725], ...
    'Outputs', '[1 1 1 1 1 1 1 1 22]');

add_block('simulink/Sources/Step', [model '/Run_Command'], ...
    'Position', [50 675 110 705], ...
    'Time', '0.02', ...
    'Before', '0', ...
    'After', '1');

add_block('simulink/Sources/Repeating Sequence', [model '/PWM_Carrier'], ...
    'Position', [500 785 590 825], ...
    'rep_seq_t', ['[0 ' halfCtrlTs ' ' ctrlTs ']'], ...
    'rep_seq_y', '[0 1 0]');

add_block('simulink/Discrete/Unit Delay', [model '/Fast_Duty_Load'], ...
    'Position', [480 585 515 615], ...
    'SampleTime', ctrlTs, ...
    'InitialCondition', '0');
add_block('simulink/Discrete/Unit Delay', [model '/Slow_Duty_Load'], ...
    'Position', [480 680 515 710], ...
    'SampleTime', ctrlTs, ...
    'InitialCondition', '0');

add_pwm_gate_logic(model, 'Fast_PWM', [535 590 805 665]);
add_static_gate_logic(model, 'Slow_State', [535 675 805 750]);

add_block('simulink/Sinks/Scope', [model '/Scope_Feedback'], ...
    'Position', [1280 585 1330 645]);
add_block('simulink/Signal Routing/Mux', [model '/Feedback_Mux'], ...
    'Position', [1215 580 1235 650], ...
    'Inputs', '3');

connect_power_stage(model);
connect_control_stage(model);

save_system(model);
end

function add_pwm_gate_logic(model, name, pos)
subsys = [model '/' name];
add_block('simulink/Ports & Subsystems/Subsystem', subsys, 'Position', pos);
delete_line_if_present(subsys, 'In1/1', 'Out1/1');
delete_block_if_present([subsys '/In1']);
delete_block_if_present([subsys '/Out1']);

add_block('simulink/Sources/In1', [subsys '/duty'], 'Position', [40 45 70 65]);
add_block('simulink/Sources/In1', [subsys '/up_en'], 'Position', [40 100 70 120]);
add_block('simulink/Sources/In1', [subsys '/dn_en'], 'Position', [40 180 70 200]);
add_block('simulink/Sources/In1', [subsys '/carrier'], 'Position', [40 250 70 270]);
add_block('simulink/Logic and Bit Operations/Relational Operator', [subsys '/carrier_lt_duty'], ...
    'Position', [145 145 185 180], 'Operator', '<');
add_block('simulink/Logic and Bit Operations/Logical Operator', [subsys '/not_pwm'], ...
    'Position', [230 215 260 245], 'Operator', 'NOT');
add_block('simulink/Logic and Bit Operations/Logical Operator', [subsys '/upper_and'], ...
    'Position', [310 85 350 125], 'Operator', 'AND');
add_block('simulink/Logic and Bit Operations/Logical Operator', [subsys '/lower_and'], ...
    'Position', [310 175 350 215], 'Operator', 'AND');
add_block('simulink/Sinks/Out1', [subsys '/upper_gate'], 'Position', [420 95 450 115]);
add_block('simulink/Sinks/Out1', [subsys '/lower_gate'], 'Position', [420 185 450 205]);

add_line(subsys, 'carrier/1', 'carrier_lt_duty/1', 'autorouting', 'on');
add_line(subsys, 'duty/1', 'carrier_lt_duty/2', 'autorouting', 'on');
add_line(subsys, 'carrier_lt_duty/1', 'upper_and/1', 'autorouting', 'on');
add_line(subsys, 'up_en/1', 'upper_and/2', 'autorouting', 'on');
add_line(subsys, 'carrier_lt_duty/1', 'not_pwm/1', 'autorouting', 'on');
add_line(subsys, 'not_pwm/1', 'lower_and/1', 'autorouting', 'on');
add_line(subsys, 'dn_en/1', 'lower_and/2', 'autorouting', 'on');
add_line(subsys, 'upper_and/1', 'upper_gate/1', 'autorouting', 'on');
add_line(subsys, 'lower_and/1', 'lower_gate/1', 'autorouting', 'on');
end

function add_static_gate_logic(model, name, pos)
subsys = [model '/' name];
add_block('simulink/Ports & Subsystems/Subsystem', subsys, 'Position', pos);
delete_line_if_present(subsys, 'In1/1', 'Out1/1');
delete_block_if_present([subsys '/In1']);
delete_block_if_present([subsys '/Out1']);

add_block('simulink/Sources/In1', [subsys '/state'], 'Position', [40 55 70 75]);
add_block('simulink/Sources/In1', [subsys '/up_en'], 'Position', [40 115 70 135]);
add_block('simulink/Sources/In1', [subsys '/dn_en'], 'Position', [40 195 70 215]);
add_block('simulink/Logic and Bit Operations/Compare To Constant', [subsys '/state_high'], ...
    'Position', [145 45 225 85], 'const', '0.5', 'relop', '>');
add_block('simulink/Logic and Bit Operations/Logical Operator', [subsys '/not_state'], ...
    'Position', [250 185 280 215], 'Operator', 'NOT');
add_block('simulink/Logic and Bit Operations/Logical Operator', [subsys '/upper_and'], ...
    'Position', [330 95 370 135], 'Operator', 'AND');
add_block('simulink/Logic and Bit Operations/Logical Operator', [subsys '/lower_and'], ...
    'Position', [330 185 370 225], 'Operator', 'AND');
add_block('simulink/Sinks/Out1', [subsys '/upper_gate'], 'Position', [430 105 460 125]);
add_block('simulink/Sinks/Out1', [subsys '/lower_gate'], 'Position', [430 195 460 215]);

add_line(subsys, 'state/1', 'state_high/1', 'autorouting', 'on');
add_line(subsys, 'state_high/1', 'upper_and/1', 'autorouting', 'on');
add_line(subsys, 'up_en/1', 'upper_and/2', 'autorouting', 'on');
add_line(subsys, 'state_high/1', 'not_state/1', 'autorouting', 'on');
add_line(subsys, 'not_state/1', 'lower_and/1', 'autorouting', 'on');
add_line(subsys, 'dn_en/1', 'lower_and/2', 'autorouting', 'on');
add_line(subsys, 'upper_and/1', 'upper_gate/1', 'autorouting', 'on');
add_line(subsys, 'lower_and/1', 'lower_gate/1', 'autorouting', 'on');
end

function connect_power_stage(model)
connect_ports(model, 'Vdc', 'RConn', 1, 'Q1_Fast_Upper', 'LConn', 1);
connect_ports(model, 'Vdc', 'RConn', 1, 'Q3_Slow_Upper', 'LConn', 1);
connect_ports(model, 'Vdc', 'LConn', 1, 'Q2_Fast_Lower', 'RConn', 1);
connect_ports(model, 'Vdc', 'LConn', 1, 'Q4_Slow_Lower', 'RConn', 1);
connect_ports(model, 'Vdc', 'LConn', 1, 'Ground', 'LConn', 1);

connect_ports(model, 'Q1_Fast_Upper', 'RConn', 1, 'Q2_Fast_Lower', 'LConn', 1);
connect_ports(model, 'Q3_Slow_Upper', 'RConn', 1, 'Q4_Slow_Lower', 'LConn', 1);

connect_ports(model, 'Q1_Fast_Upper', 'RConn', 1, 'I_L_Meas', 'LConn', 1);
connect_ports(model, 'I_L_Meas', 'RConn', 1, 'L_Filter', 'LConn', 1);
connect_ports(model, 'L_Filter', 'RConn', 1, 'C_Filter', 'LConn', 1);
connect_ports(model, 'L_Filter', 'RConn', 1, 'R_Load', 'LConn', 1);
connect_ports(model, 'C_Filter', 'RConn', 1, 'Q3_Slow_Upper', 'RConn', 1);
connect_ports(model, 'R_Load', 'RConn', 1, 'Q3_Slow_Upper', 'RConn', 1);

connect_ports(model, 'V_Cap_Meas', 'LConn', 1, 'L_Filter', 'RConn', 1);
connect_ports(model, 'V_Cap_Meas', 'LConn', 2, 'Q3_Slow_Upper', 'RConn', 1);
connect_ports(model, 'V_Bus_Meas', 'LConn', 1, 'Vdc', 'RConn', 1);
connect_ports(model, 'V_Bus_Meas', 'LConn', 2, 'Vdc', 'LConn', 1);
end

function connect_control_stage(model)
add_line(model, 'I_L_Meas/1', 'Input_Mux/1', 'autorouting', 'on');
add_line(model, 'V_Cap_Meas/1', 'Input_Mux/2', 'autorouting', 'on');
add_line(model, 'V_Bus_Meas/1', 'Input_Mux/3', 'autorouting', 'on');
add_line(model, 'Run_Command/1', 'Input_Mux/4', 'autorouting', 'on');
add_line(model, 'Input_Mux/1', 'sfunc/1', 'autorouting', 'on');
add_line(model, 'sfunc/1', 'Output_Demux/1', 'autorouting', 'on');

add_line(model, 'Output_Demux/1', 'Fast_Duty_Load/1', 'autorouting', 'on');
add_line(model, 'Fast_Duty_Load/1', 'Fast_PWM/1', 'autorouting', 'on');
add_line(model, 'Output_Demux/2', 'Fast_PWM/2', 'autorouting', 'on');
add_line(model, 'Output_Demux/3', 'Fast_PWM/3', 'autorouting', 'on');
add_line(model, 'PWM_Carrier/1', 'Fast_PWM/4', 'autorouting', 'on');
add_line(model, 'Fast_PWM/1', 'Q1_Fast_Upper/1', 'autorouting', 'on');
add_line(model, 'Fast_PWM/2', 'Q2_Fast_Lower/1', 'autorouting', 'on');

add_line(model, 'Output_Demux/4', 'Slow_Duty_Load/1', 'autorouting', 'on');
add_line(model, 'Slow_Duty_Load/1', 'Slow_State/1', 'autorouting', 'on');
add_line(model, 'Output_Demux/5', 'Slow_State/2', 'autorouting', 'on');
add_line(model, 'Output_Demux/6', 'Slow_State/3', 'autorouting', 'on');
add_line(model, 'Slow_State/1', 'Q3_Slow_Upper/1', 'autorouting', 'on');
add_line(model, 'Slow_State/2', 'Q4_Slow_Lower/1', 'autorouting', 'on');

add_line(model, 'I_L_Meas/1', 'Feedback_Mux/1', 'autorouting', 'on');
add_line(model, 'V_Cap_Meas/1', 'Feedback_Mux/2', 'autorouting', 'on');
add_line(model, 'V_Bus_Meas/1', 'Feedback_Mux/3', 'autorouting', 'on');
add_line(model, 'Feedback_Mux/1', 'Scope_Feedback/1', 'autorouting', 'on');
end

function connect_ports(model, srcBlock, srcField, srcIndex, dstBlock, dstField, dstIndex)
srcHandles = get_param([model '/' srcBlock], 'PortHandles');
dstHandles = get_param([model '/' dstBlock], 'PortHandles');
srcPort = srcHandles.(srcField);
dstPort = dstHandles.(dstField);
if numel(srcPort) > 1
    srcPort = srcPort(srcIndex);
end
if numel(dstPort) > 1
    dstPort = dstPort(dstIndex);
end
add_line(model, srcPort, dstPort, 'autorouting', 'on');
end

function delete_block_if_present(block)
if getSimulinkBlockHandle(block) >= 0
    delete_block(block);
end
end

function delete_line_if_present(system, src, dst)
try
    delete_line(system, src, dst);
catch
end
end
