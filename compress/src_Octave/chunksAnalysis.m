% ====================================================
%
% Make some statistical comparaison between 
% different size of chunks of entry data
% using the LZ4 compression algorithm
%
% CONUS Vincent
% 29.06.2017
%
% ====================================================   


% ###################################################################

scanTitle = 'Test chunks of Totscan raw data';

pkg load statistics % package useful for boxplot and histograms

data5M = csvread("../data/chunked/5M.csv");
data2M = csvread("../data/chunked/2M.csv");
data1M = csvread("../data/chunked/1M.csv");
data500k = csvread("../data/chunked/500k.csv");
data300k = csvread("../data/chunked/300k.csv");
data200k = csvread("../data/chunked/200k.csv");
data100k = csvread("../data/chunked/100k.csv");


compr5M = 1-(data5M(1:end, 2:end-3) ./ data5M(1:end, 1)); % doesn't take the last small chunk 
compr5M = reshape(compr5M, 1, []);

compr2M = 1-(data2M(1:end, 2:end-3) ./ data2M(1:end, 1)); % doesn't take the last small chunk 
compr2M = reshape(compr2M, 1, []);

compr1M = 1-(data1M(1:end, 2:end-3) ./ data1M(1:end, 1)); % doesn't take the last small chunk 
compr1M = reshape(compr1M, 1, []);

compr500k = 1-(data500k(1:end, 2:end-3) ./ data500k(1:end, 1)); % doesn't take the last small chunk 
compr500k = reshape(compr500k, 1, []);

compr300k = 1-(data300k(1:end, 2:end-3) ./ data300k(1:end, 1)); % doesn't take the last small chunk 
compr300k = reshape(compr300k, 1, []);

compr200k = 1-(data200k(1:end, 2:end-3) ./ data200k(1:end, 1)); % doesn't take the last small chunk 
compr200k = reshape(compr200k, 1, []);

compr100k = 1-(data100k(1:end, 2:end-3) ./ data100k(1:end, 1)); % doesn't take the last small chunk 
compr100k = reshape(compr100k, 1, []);

compr = {compr5M, compr2M, compr1M, compr500k, compr300k, compr200k, compr100k};

meancompr = [mean(compr5M), mean(compr2M), mean(compr1M), mean(compr500k), mean(compr300k), mean(compr200k), mean(compr100k)];
medcompr = [median(compr5M), median(compr2M), median(compr1M), median(compr500k), median(compr300k), median(compr200k), median(compr100k)];


% -------------------------------------------------------------------

time5Mcom = data5M(1:end, end-1);
time2Mcom = data2M(1:end, end-1);
time1Mcom = data1M(1:end, end-1);
time500kcom = data500k(1:end, end-1);
time300kcom = data300k(1:end, end-1);
time200kcom = data200k(1:end, end-1);
time100kcom = data100k(1:end, end-1);

time5Mucom = data5M(1:end, end);
time2Mucom = data2M(1:end, end);
time1Mucom = data1M(1:end, end);
time500kucom = data500k(1:end, end);
time300kucom = data300k(1:end, end);
time200kucom = data200k(1:end, end);
time100kucom = data100k(1:end, end);

comptime = {time5Mcom, time2Mcom, time1Mcom, time500kcom, time300kcom, time200kcom, time100kcom};

decomptime = {time5Mucom, time2Mucom, time1Mucom, time500kucom, time300kucom, time200kucom, time100kucom};

% ###################################################################
figure
subplot(3, 1, 1)
hold on;
plot(meancompr, '-xc');
plot(medcompr, '-xk');
boxplot(compr);

set(gca, 'YTickMode','manual');
set(gca, 'YTickLabel',num2str(100.*get(gca,'YTick')','%g%%'));
grid on;

subplot(3, 1, 2)
hold on;
grid on;
boxplot(comptime);

subplot(3, 1, 3)
hold on;
grid on;
boxplot(decomptime);