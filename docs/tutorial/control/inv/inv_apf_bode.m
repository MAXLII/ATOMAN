%% 单相逆变器一阶全通滤波器波特图
% 连续域传递函数：
%
%                 omega_f - s
% H_apf(s) = -----------------------
%                 omega_f + s
%
% 其中 omega_f 为基频角频率。脚本同时绘制连续域响应和 code/lib/apf.c
% 使用 Tustin 变换得到的离散域响应，便于检查数字实现与理论模型的一致性。
clear; clc; close all;

%% 1. APF 与扫频参数
fundamental_freq_hz = 50;
omega_f = 2 * pi * fundamental_freq_hz;
sample_freq_hz = 30e3;
sample_time_s = 1 / sample_freq_hz;

freq_hz = logspace(0, 4, 2000);
omega_radps = 2 * pi * freq_hz;

%% 2. 连续域频率响应 H(jw) = (omega_f - jw)/(omega_f + jw)
h_continuous = (omega_f - 1j * omega_radps) ./ ...
               (omega_f + 1j * omega_radps);
magnitude_continuous_db = 20 * log10(abs(h_continuous));
phase_continuous_deg = unwrap(angle(h_continuous)) * 180 / pi;

%% 3. 与 code/lib/apf.c 一致的 Tustin 离散域频率响应
b0 = ((omega_f * sample_time_s) - 2) / ...
     ((omega_f * sample_time_s) + 2);
b1 = 1;
a1 = b0;
z_inverse = exp(-1j * omega_radps * sample_time_s);
h_discrete = (b0 + b1 * z_inverse) ./ (1 + a1 * z_inverse);
magnitude_discrete_db = 20 * log10(abs(h_discrete));
phase_discrete_deg = unwrap(angle(h_discrete)) * 180 / pi;

%% 4. 基频处的幅值和相位
h_fundamental_continuous = (omega_f - 1j * omega_f) / ...
                           (omega_f + 1j * omega_f);
z_fundamental_inverse = exp(-1j * omega_f * sample_time_s);
h_fundamental_discrete = (b0 + b1 * z_fundamental_inverse) / ...
                         (1 + a1 * z_fundamental_inverse);

fundamental_magnitude_continuous_db = 20 * log10(abs(h_fundamental_continuous));
fundamental_phase_continuous_deg = angle(h_fundamental_continuous) * 180 / pi;
fundamental_magnitude_discrete_db = 20 * log10(abs(h_fundamental_discrete));
fundamental_phase_discrete_deg = angle(h_fundamental_discrete) * 180 / pi;

fprintf('First-order APF: H(s) = (omega_f - s)/(omega_f + s)\n');
fprintf('Fundamental frequency: %.3f Hz\n', fundamental_freq_hz);
fprintf('Continuous response : magnitude=%+.6f dB, phase=%+.6f deg\n', ...
        fundamental_magnitude_continuous_db, fundamental_phase_continuous_deg);
fprintf('Discrete response   : magnitude=%+.6f dB, phase=%+.6f deg\n', ...
        fundamental_magnitude_discrete_db, fundamental_phase_discrete_deg);

assert(max(abs(magnitude_continuous_db)) < 1e-10, ...
       'The continuous all-pass magnitude must remain at 0 dB.');
assert(max(abs(magnitude_discrete_db)) < 1e-10, ...
       'The discrete all-pass magnitude must remain at 0 dB.');

%% 5. 绘制波特图
figure_handle = figure('Color', 'w', 'Name', 'First-order APF Bode Plot');
tiledlayout(2, 1, 'TileSpacing', 'compact', 'Padding', 'compact');

nexttile;
semilogx(freq_hz, magnitude_continuous_db, 'LineWidth', 1.8);
hold on;
semilogx(freq_hz, magnitude_discrete_db, '--', 'LineWidth', 1.5);
plot(fundamental_freq_hz, fundamental_magnitude_continuous_db, ...
     'o', 'MarkerSize', 7, 'LineWidth', 1.5);
xline(fundamental_freq_hz, ':', '50 Hz', 'LabelVerticalAlignment', 'bottom');
grid on;
xlim([freq_hz(1), freq_hz(end)]);
ylim([-0.02, 0.02]);
ylabel('Magnitude (dB)');
title('Magnitude response');
legend('Continuous APF', 'Tustin APF (30 kHz)', '50 Hz operating point', ...
       'Location', 'southwest');

nexttile;
semilogx(freq_hz, phase_continuous_deg, 'LineWidth', 1.8);
hold on;
semilogx(freq_hz, phase_discrete_deg, '--', 'LineWidth', 1.5);
plot(fundamental_freq_hz, fundamental_phase_continuous_deg, ...
     'o', 'MarkerSize', 7, 'LineWidth', 1.5);
xline(fundamental_freq_hz, ':', '50 Hz', 'LabelVerticalAlignment', 'bottom');
yline(-90, ':', '-90 deg', 'LabelHorizontalAlignment', 'left');
grid on;
xlim([freq_hz(1), freq_hz(end)]);
ylim([-190, 10]);
xlabel('Frequency (Hz)');
ylabel('Phase (deg)');
title('Phase response');
legend('Continuous APF', 'Tustin APF (30 kHz)', '50 Hz operating point', ...
       'Location', 'southwest');

sgtitle('First-order APF: H(s) = (\omega_f - s)/(\omega_f + s),  f_f = 50 Hz');

result_file = fullfile(tempdir, 'inv_apf_bode.png');
exportgraphics(figure_handle, result_file, 'Resolution', 180);
fprintf('Result figure: %s\n', result_file);
