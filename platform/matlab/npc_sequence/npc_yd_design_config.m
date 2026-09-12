function p=npc_yd_design_config()
% PI synchronized with npc_cfg_default; unloading acceptance remains incomplete.
% Analysis ratings do not establish hardware ratings.
m=npc_yd_model; p=m.p;
% Joint tuning includes uncertain dynamic modulation loss; see NPC_OSCILLATION_PI_REDESIGN.md.
p.kpv=1.0; p.kiv=1300.0;
p.kpi=0.15; p.kii=0.05;
p.ipeak=5200; % Includes the 4124 A steady sequence requirement and startup reference headroom.
p.duration=20; p.ramp_time=2; p.event_time=8;
p.guard_enabled=false; % Diagnostic stress tests can continue to expose physical overshoot.
p.trip_current=1.2*p.ipeak; p.trip_voltage=1.2*p.vpeak;
% Guards halt this averaged simulation on a fault. They do not model diode turn-off/energy disposal.
end
