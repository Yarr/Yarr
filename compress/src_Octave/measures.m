
% ====================================================
%
% Separated file that contains
% the data file calls and the 
% statistical operation on them
%
% CONUS Vincent
% 01.06.2017
%
% ====================================================   


pkg load statistics % package useful for boxplot and histograms

size = csvread("../data/z_not_written_now/5_Correct_Tot_data_LZ4/strong/uncompSize.csv");
lz4data = csvread("../data/z_not_written_now/5_Correct_Tot_data_LZ4/strong/lz4.csv");
lz4size = lz4data(:,2); % compressed size
lz49data = csvread("../data/z_not_written_now/5_Correct_Tot_data_LZ4/strong/lz49.csv");
lz49size = lz49data(:,2); % compressed size
size = vertcat(size, lz4data(:,1), lz49data(:,1)); % add the raw data size measured with lz4

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

% for LZ4 strong but slower

lz49meansize = mean(lz49size);
lz49mediansize = median(lz49size);
lz49stdsize = std(lz49size);
lz49graphspan = [0 length(lz49size)]; % vector with the size of the data. 