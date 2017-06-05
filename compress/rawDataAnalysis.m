
% ====================================================
%
% Script to make some statistic tests on the raw datas
% and to compressed data.
%
% CONUS Vincent
% 01.06.2017
%
% ====================================================   

pkg load statistics % package useful for boxplot and histograms


size = csvread("./data/uncompSize.csv");
lz4data = csvread("./data/lz4.csv");
lz4size = lz4data(:,2); % compressed size
size = vertcat(size, lz4data(:,1)); % add the raw data size measured with lz4

% for raw data
meansize = mean(size);
mediansize = median(size);
stdsize = std(size);
graphspan = [0 length(size)]; % vector with the size of the data. 

% for LZ4

lz4meansize = mean(lz4size);
lz4mediansize = median(lz4size);
lz4stdsize = std(lz4size);
lz4graphspan = [0 length(lz4size)]; % vector with the size of the data. 


%% ===========================================================================
%% ===========================================================================
figure('Position', [20 20 1300 740]); % 720p window
%% ===========================================================================
% no compression plot
%% ===========================================================================
subplot(2, 2, 1)
plot(size, '-x', 'linewidth', 0.7); % plot raw size
hold on;

plot(graphspan, [meansize meansize], 'g', 'linewidth', 2); % plot mean
plot(graphspan, [mediansize mediansize], 'r', 'linewidth', 2); % plot median
% standard deviation
plot(graphspan, [meansize+stdsize meansize+stdsize], 'k', 'linewidth', 2);
plot(graphspan, [meansize-stdsize meansize-stdsize], 'k', 'linewidth', 2);


% -----------------------------------
%axis([graphspan 7e6 8e6]);
grid on;
ylabel 'Raw data size [Byte]'
xlabel 'Iterations of size measurment'

title("Raw TotScan uncompressed data sizes", 'FontWeight', 'bold', 'FontSize', 13);
legend(sprintf("Raw data size value"), ...
       sprintf("Mean = %3.2f MB", meansize/1000000), ...
       sprintf("Median = %3.2f MB", mediansize/1000000), ...
       sprintf("Standard deviation = %3.2f MB (%3.3d %%)", stdsize/1000000, (stdsize/meansize)*100),...
       'Location', 'southwest'
);



%% =========================================================================== 
% LZ4 plot
%% =========================================================================== 
subplot(2, 2, 2)
plot(lz4size, '-x', 'linewidth', 0.7); % plot raw size
hold on;

plot(lz4graphspan, [lz4meansize lz4meansize], 'g', 'linewidth', 2); % plot mean
plot(lz4graphspan, [lz4mediansize lz4mediansize], 'r', 'linewidth', 2); % plot median
% standard deviation
plot(lz4graphspan, [lz4meansize+lz4stdsize lz4meansize+lz4stdsize], 'k', 'linewidth', 2);
plot(lz4graphspan, [lz4meansize-lz4stdsize lz4meansize-lz4stdsize], 'k', 'linewidth', 2);

% -----------------------------------
%axis([lz4graphspan 2e6 1.2e7]);
grid on;
ylabel 'LZ4 compressed data size [Byte]'
xlabel 'Iterations of compressions'

legend(sprintf("LZ4 compressed data size value"), ...
       sprintf("Mean = %3.3f MB", lz4meansize/1000000), ...
       sprintf("Median = %3.3f MB", lz4mediansize/1000000), ...
       sprintf("Standard deviation = %3.3f MB (%3.3d %%)", lz4stdsize/1000000, (lz4stdsize/lz4meansize)*100),...
       'Location', 'southwest'
);
title("LZ4 compressed data sizes", 'FontWeight', 'bold', 'FontSize', 13);


%% ===========================================================================
% Gain
%% ===========================================================================
subplot(2, 2, 3)

gain_lz4 = 1- (lz4data(:,2)./lz4data(:,1));
gainlz4meansize = mean(gain_lz4);
gainlz4mediansize = median(gain_lz4);
gainlz4stdsize = std(gain_lz4);

plot(gain_lz4, '-x', 'linewidth', 0.7);
hold on;

plot(lz4graphspan, [gainlz4meansize gainlz4meansize], 'g', 'linewidth', 2); % plot mean
plot(lz4graphspan, [gainlz4mediansize gainlz4mediansize], 'r', 'linewidth', 2); % plot median
% standard deviation
plot(lz4graphspan, [gainlz4meansize+gainlz4stdsize gainlz4meansize+gainlz4stdsize], 'k', 'linewidth', 2);
plot(lz4graphspan, [gainlz4meansize-gainlz4stdsize gainlz4meansize-gainlz4stdsize], 'k', 'linewidth', 2);

% -----------------------------------
%axis([lz4graphspan 0.974 0.989]);
grid on;
ylabel 'Compression gain'
xlabel 'Iterations of compressions'
set(gca, 'YTickMode','manual')
set(gca, 'YTickLabel',num2str(100.*get(gca,'YTick')','%g%%'))
legend(sprintf("LZ4 compression gain"), ...
       sprintf("Mean = %d %%", gainlz4meansize*100), ...
       sprintf("Median = %d %%", gainlz4mediansize*100), ...
       sprintf("Standard deviation = %d %% (%3.3d %%)", gainlz4stdsize*100, (gainlz4stdsize/gainlz4meansize)*100),...
       'Location', 'southwest'
);
title ("LZ4 compression gain", 'FontWeight', 'bold', 'FontSize', 13);


%% ===========================================================================
% Compression / decompression time
%% ===========================================================================
subplot(2, 2, 4)

plot(lz4data(:,3), '-x', 'linewidth', 0.7); % plot cmpression time
hold on;
plot(lz4data(:,4), '-xr', 'linewidth', 0.7); % plot cmpression time

lz4meancomp=mean(lz4data(:,3));
lz4meandecomp=mean(lz4data(:,4));

plot(lz4graphspan, [lz4meancomp lz4meancomp], 'c', 'linewidth', 2);
plot(lz4graphspan, [lz4meandecomp lz4meandecomp], 'm', 'linewidth', 2);


% -----------------------------------
%axis([lz4graphspan 0.974 0.989]);
grid on;
ylabel 'Duration [ns]'
xlabel 'Iterations of compressions or decompression'
legend(sprintf("Compression"), ...
       sprintf("Decompression") , ...
       sprintf("Comp. mean = %3.3d us", lz4meancomp/1000000),...     
       sprintf("Deomp. mean = %3.3d us", lz4meandecomp/1000000),...
       'Location', 'northwest'
);
title ("LZ4 compression and decompression time", 'FontWeight', 'bold', 'FontSize', 13);


%% EOF

