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

totalSize = 10777600;

pkg load statistics % package useful for boxplot and histograms

data5M = csvread("../data/chunked/5M.csv");
data2M = csvread("../data/chunked/2M.csv");
data1M = csvread("../data/chunked/1M.csv");
data500k = csvread("../data/chunked/500k.csv");
data300k = csvread("../data/chunked/300k.csv");
data200k = csvread("../data/chunked/200k.csv");
data100k = csvread("../data/chunked/100k.csv");
data50k = csvread("../data/chunked/50k.csv");
data20k = csvread("../data/chunked/20k.csv");
data10k = csvread("../data/chunked/10k.csv");

sizes = [5e6, 2e6, 1e6, 500e3, 300e3, 200e3, 100e3, 50e3, 20e3, 10e3];


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

compr50k = 1-(data50k(1:end, 2:end-3) ./ data50k(1:end, 1)); % doesn't take the last small chunk 
compr50k = reshape(compr50k, 1, []);

compr20k = 1-(data20k(1:end, 2:end-3) ./ data20k(1:end, 1)); % doesn't take the last small chunk 
compr20k = reshape(compr20k, 1, []);

compr10k = 1-(data10k(1:end, 2:end-3) ./ data10k(1:end, 1)); % doesn't take the last small chunk 
compr10k = reshape(compr10k, 1, []);

compr = {compr5M, compr2M, compr1M, compr500k, compr300k, compr200k, compr100k, compr50k, compr20k, compr10k};

meancompr = [mean(compr5M), mean(compr2M), mean(compr1M), ...
             mean(compr500k), mean(compr300k), mean(compr200k), mean(compr100k), ...
             mean(compr50k), mean(compr20k), mean(compr10k)];
medcompr = [median(compr5M), median(compr2M), median(compr1M), ...
            median(compr500k), median(compr300k), median(compr200k), median(compr100k), ...
            median(compr50k), median(compr20k), median(compr10k)];
           
iqrcompr = [iqr(compr5M), iqr(compr2M), iqr(compr1M), ...
            iqr(compr500k), iqr(compr300k), iqr(compr200k), iqr(compr100k), ...
            iqr(compr50k), iqr(compr20k), iqr(compr10k)];


%% comp against the full size of the data
fullcompr5M = 1-(sum(data5M(1:end, 2:end-2)') ./ totalSize);
fullcompr2M = 1-(sum(data2M(1:end, 2:end-2)') ./ totalSize);
fullcompr1M = 1-(sum(data1M(1:end, 2:end-2)') ./ totalSize);
fullcompr500k = 1-(sum(data500k(1:end, 2:end-2)') ./ totalSize);
fullcompr300k = 1-(sum(data300k(1:end, 2:end-2)') ./ totalSize);
fullcompr200k = 1-(sum(data200k(1:end, 2:end-2)') ./ totalSize);
fullcompr100k = 1-(sum(data100k(1:end, 2:end-2)') ./ totalSize);
fullcompr50k = 1-(sum(data50k(1:end, 2:end-2)') ./ totalSize);
fullcompr20k = 1-(sum(data20k(1:end, 2:end-2)') ./ totalSize);
fullcompr10k = 1-(sum(data10k(1:end, 2:end-2)') ./ totalSize);

fullmeancompr = [mean(fullcompr5M), mean(fullcompr2M), mean(fullcompr1M), ...
             mean(fullcompr500k), mean(fullcompr300k), mean(fullcompr200k), mean(fullcompr100k), ...
             mean(fullcompr50k), mean(fullcompr20k), mean(fullcompr10k)];
fullmedcompr = [median(fullcompr5M), median(fullcompr2M), median(fullcompr1M), ...
            median(fullcompr500k), median(fullcompr300k), median(fullcompr200k), median(fullcompr100k), ...
            median(fullcompr50k), median(fullcompr20k), median(fullcompr10k)];

iqrfullmedcompr = [iqr(fullcompr5M), iqr(fullcompr2M), iqr(fullcompr1M), ...
                   iqr(fullcompr500k), iqr(fullcompr300k), iqr(fullcompr200k), iqr(fullcompr100k), ...
                   iqr(fullcompr50k), iqr(fullcompr20k), iqr(fullcompr10k)];

% -------------------------------------------------------------------

time5Mcom = data5M(1:end, end-1);
time2Mcom = data2M(1:end, end-1);
time1Mcom = data1M(1:end, end-1);
time500kcom = data500k(1:end, end-1);
time300kcom = data300k(1:end, end-1);
time200kcom = data200k(1:end, end-1);
time100kcom = data100k(1:end, end-1);
time50kcom = data50k(1:end, end-1);
time20kcom = data20k(1:end, end-1);
time10kcom = data10k(1:end, end-1);

time5Mucom = data5M(1:end, end);
time2Mucom = data2M(1:end, end);
time1Mucom = data1M(1:end, end);
time500kucom = data500k(1:end, end);
time300kucom = data300k(1:end, end);
time200kucom = data200k(1:end, end);
time100kucom = data100k(1:end, end);
time50kucom = data50k(1:end, end);
time20kucom = data20k(1:end, end);
time10kucom = data10k(1:end, end);

comptime = {time5Mcom, time2Mcom, time1Mcom, time500kcom, time300kcom, time200kcom, time100kcom, time50kcom, time20kcom, time10kcom};

decomptime = {time5Mucom, time2Mucom, time1Mucom, time500kucom, time300kucom, time200kucom, time100kucom, time50kucom, time20kucom, time10kucom};


comptimemedian = [median(time5Mcom), median(time2Mcom), median(time1Mcom), ...
               median(time500kcom), median(time300kcom), median(time200kcom), median(time100kcom), ...
               median(time50kcom), median(time20kcom), median(time10kcom)];

decomptimemedian = [median(time5Mucom), median(time2Mucom), median(time1Mucom), ...
                 median(time500kucom), median(time300kucom), median(time200kucom), median(time100kucom), ...
                 median(time50kucom), median(time20kucom), median(time10kucom)];


comptimeiqr = [iqr(time5Mcom), iqr(time2Mcom), iqr(time1Mcom), ...
               iqr(time500kcom), iqr(time300kcom), iqr(time200kcom), iqr(time100kcom), ...
               iqr(time50kcom), iqr(time20kcom), iqr(time10kcom)];

decomptimeiqr = [iqr(time5Mucom), iqr(time2Mucom), iqr(time1Mucom), ...
                 iqr(time500kucom), iqr(time300kucom), iqr(time200kucom), iqr(time100kucom), ...
                 iqr(time50kucom), iqr(time20kucom), iqr(time10kucom)];


% ###################################################################
figure
subplot(2, 2, 1)
hold on;
plot(sizes./1000, meancompr, '-xr');
%plot(sizes./1000, medcompr, '-xg');
errorbar(sizes./1000, medcompr, iqrcompr);

legend('Mean', 'Median', 'location', 'SouthEast')

set(gca,'XScale','log');
set(gca, 'YTickMode','manual');
set(gca, 'YTickLabel',num2str(100.*get(gca,'YTick')','%g%%'));
grid on;

%xlabel("Chunks size [kB]");
ylabel("Compression rate");

title("Compression for one chunk")

% -------------------------------------------------------------------

subplot(2, 2, 2)
hold on;
plot(sizes./1000, fullmeancompr, '-xr');
%plot(sizes./1000, fullmedcompr, '-xg');
errorbar(sizes./1000, fullmedcompr, iqrfullmedcompr);

legend('Mean', 'Median', 'location', 'SouthEast')

set(gca,'XScale','log');
set(gca, 'YTickMode','manual');
set(gca, 'YTickLabel',num2str(100.*get(gca,'YTick')','%g%%'));
grid on;

%xlabel("Chunks size [kB]");
ylabel("Compression rate");

title("Compression for the whole data")

% -------------------------------------------------------------------

subplot(2, 2, 3)
hold on;
errorbar(sizes./1000, comptimemedian./1e6, comptimeiqr./1e6);

set(gca,'XScale','log');
grid on;

xlabel("Chunks size [kB]");
ylabel("Duration [ms]");

title("Compression");

% -------------------------------------------------------------------

subplot(2, 2, 4)
hold on;
errorbar(sizes./1000, decomptimemedian./1e6, decomptimeiqr./1e6);

set(gca,'XScale','log');
grid on;

xlabel("Chunks size [kB]");
ylabel("Duration [ms]");

title("Decompression");