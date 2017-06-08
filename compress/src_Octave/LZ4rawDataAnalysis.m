
% ====================================================
%
% Make some statistical comparaison between two LZ4
% compression mode.
%
% CONUS Vincent
% 01.06.2017
%
% ====================================================   

pkg load statistics % package useful for boxplot and histograms

measures


%% ===========================================================================
%% ===========================================================================
figure('Position', [20 20 1300 740]); % 720p window
%% ===========================================================================
subplot(2, 4, [1, 2]) % no compression plot
  
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

  title("Raw DigitalScan uncompressed data sizes", 'FontWeight', 'bold', 'FontSize', 13);
  legend(sprintf("Raw data size value"), ...
         sprintf("Mean = %3.2f MB", meansize/1000000), ...
         sprintf("Median = %3.2f MB", mediansize/1000000), ...
         sprintf("Standard deviation = %3.2f MB (%3.3d %%)", stdsize/1000000, (stdsize/meansize)*100),...
         'Location', 'southwest'
  );
  


%% =========================================================================== 
subplot(2, 4, 3) % LZ4 plot
  plot(lz4size, '-x', 'linewidth', 0.7); % plot raw size
  hold on;

  plot(lz4graphspan, [lz4meansize lz4meansize], 'g', 'linewidth', 2); % plot mean
  plot(lz4graphspan, [lz4mediansize lz4mediansize], 'r', 'linewidth', 2); % plot median
  % standard deviation
  plot(lz4graphspan, [lz4meansize+lz4stdsize lz4meansize+lz4stdsize], 'k', 'linewidth', 2);
  plot(lz4graphspan, [lz4meansize-lz4stdsize lz4meansize-lz4stdsize], 'k', 'linewidth', 2);

  % -----------------------------------
  axis([lz4graphspan 1.4e5 1.8e5]);
  grid on;
  ylabel 'LZ4 compressed data size [Byte]'
  xlabel 'Iterations of compressions'

  legend(sprintf("LZ4 compressed data size value"), ...
         sprintf("Mean = %3.3f MB", lz4meansize/1000000), ...
         sprintf("Median = %3.3f MB", lz4mediansize/1000000), ...
         sprintf("Std = %3.3f MB (%3.3d %%)", lz4stdsize/1000000, (lz4stdsize/lz4meansize)*100),...
         'Location', 'southwest'
  );
  title("LZ4 compressed data sizes", 'FontWeight', 'bold', 'FontSize', 13);


%% =========================================================================== 
subplot(2, 4, 4) % LZ4 strong plot
  plot(lz49size, '-x', 'linewidth', 0.7); % plot raw size
  hold on;

  plot(lz49graphspan, [lz49meansize lz49meansize], 'g', 'linewidth', 2); % plot mean
  plot(lz49graphspan, [lz49mediansize lz49mediansize], 'r', 'linewidth', 2); % plot median
  % standard deviation
  plot(lz49graphspan, [lz49meansize+lz49stdsize lz49meansize+lz49stdsize], 'k', 'linewidth', 2);
  plot(lz49graphspan, [lz49meansize-lz49stdsize lz49meansize-lz49stdsize], 'k', 'linewidth', 2);

  % -----------------------------------
  axis([lz49graphspan 1.4e5 1.8e5]);
  grid on;
  ylabel 'LZ4 compressed data size [Byte]'
  xlabel 'Iterations of compressions'

  legend(sprintf("LZ4 compressed data size value"), ...
         sprintf("Mean = %3.3f MB", lz49meansize/1000000), ...
         sprintf("Median = %3.3f MB", lz49mediansize/1000000), ...
         sprintf("Std = %3.3f MB (%3.3d %%)", lz49stdsize/1000000, (lz49stdsize/lz49meansize)*100),...
         'Location', 'southwest'
  );
  title("LZ4 strong comp. data sizes", 'FontWeight', 'bold', 'FontSize', 13);

  
%% ===========================================================================
subplot(2, 4, 5) % Gain

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
  %axis([lz4graphspan 0.983 0.9855]);
  grid on;
  ylabel 'Compression gain'
  xlabel 'Iterations of compressions'
  set(gca, 'YTickMode','manual')
  set(gca, 'YTickLabel',num2str(100.*get(gca,'YTick')','%g%%'))
  legend(sprintf("LZ4 compression gain"), ...
         sprintf("Mean = %d %%", gainlz4meansize*100), ...
         sprintf("Median = %d %%", gainlz4mediansize*100), ...
         sprintf("Std = %d %% (%3.3d %%)", gainlz4stdsize*100, (gainlz4stdsize/gainlz4meansize)*100),...
         'Location', 'southwest'
  );
  title ("LZ4 compression gain", 'FontWeight', 'bold', 'FontSize', 13);

%% ===========================================================================
subplot(2, 4, 6) % Gain strong LZ4

  gain_lz49 = 1- (lz49data(:,2)./lz49data(:,1));
  gainlz49meansize = mean(gain_lz49);
  gainlz49mediansize = median(gain_lz49);
  gainlz49stdsize = std(gain_lz49);

  plot(gain_lz49, '-x', 'linewidth', 0.7);
  hold on;

  plot(lz49graphspan, [gainlz49meansize gainlz49meansize], 'g', 'linewidth', 2); % plot mean
  plot(lz49graphspan, [gainlz49mediansize gainlz49mediansize], 'r', 'linewidth', 2); % plot median
  % standard deviation
  plot(lz49graphspan, [gainlz49meansize+gainlz49stdsize gainlz49meansize+gainlz49stdsize], 'k', 'linewidth', 2);
  plot(lz49graphspan, [gainlz49meansize-gainlz49stdsize gainlz49meansize-gainlz49stdsize], 'k', 'linewidth', 2);

  % -----------------------------------
  %axis([lz49graphspan 0.983 0.9855]);
  grid on;
  ylabel 'Compression gain'
  xlabel 'Iterations of compressions'
  set(gca, 'YTickMode','manual')
  set(gca, 'YTickLabel',num2str(100.*get(gca,'YTick')','%g%%'))
  legend(sprintf("LZ4 compression gain"), ...
         sprintf("Mean = %d %%", gainlz49meansize*100), ...
         sprintf("Median = %d %%", gainlz49mediansize*100), ...
         sprintf("Std = %d %% (%3.3d %%)", gainlz49stdsize*100, (gainlz49stdsize/gainlz49meansize)*100),...
         'Location', 'southwest'
  );
  title ("LZ4 strong comp. gain", 'FontWeight', 'bold', 'FontSize', 13);


%% ===========================================================================
subplot(2, 4, 7) % Compression / decompression time

  plot(lz4data(:,3), '-x', 'linewidth', 0.7); % plot cmpression time
  hold on;
  plot(lz4data(:,4), '-xr', 'linewidth', 0.7); % plot cmpression time

  lz4meancomp=mean(lz4data(:,3));
  lz4meandecomp=mean(lz4data(:,4));

  plot(lz4graphspan, [lz4meancomp lz4meancomp], 'c', 'linewidth', 2);
  plot(lz4graphspan, [lz4meandecomp lz4meandecomp], 'm', 'linewidth', 2);


  % -----------------------------------
  axis([lz4graphspan 9e6 6.5e7]);
  grid on;
  ylabel 'Duration [ns]'
  xlabel 'Iterations of compressions or decompression'
  legend(sprintf("Compression"), ...
         sprintf("Decompression") , ...
         sprintf("Comp. mean = %3.3d us", lz4meancomp/1000000),...     
         sprintf("Deomp. mean = %3.3d us", lz4meandecomp/1000000),...
         'Location', 'northwest'
  );
  title ("LZ4 comp. and decomp. time", 'FontWeight', 'bold', 'FontSize', 13);

%% ===========================================================================
subplot(2, 4, 8) % Compression / decompression time for strong LZ4

  plot(lz49data(:,3), '-x', 'linewidth', 0.7); % plot cmpression time
  hold on;
  plot(lz49data(:,4), '-xr', 'linewidth', 0.7); % plot cmpression time

  lz49meancomp=mean(lz49data(:,3));
  lz49meandecomp=mean(lz49data(:,4));

  plot(lz49graphspan, [lz49meancomp lz49meancomp], 'c', 'linewidth', 2);
  plot(lz49graphspan, [lz49meandecomp lz49meandecomp], 'm', 'linewidth', 2);


  % -----------------------------------
  axis([lz49graphspan 9e6 6.5e7]);
  grid on;
  ylabel 'Duration [ns]'
  xlabel 'Iterations of compressions or decompression'
  legend(sprintf("Compression"), ...
         sprintf("Decompression") , ...
         sprintf("Comp. mean = %3.3d us", lz49meancomp/1000000),...     
         sprintf("Deomp. mean = %3.3d us", lz49meandecomp/1000000),...
         'Location', 'northwest'
  );
  title ("LZ4 strong time", 'FontWeight', 'bold', 'FontSize', 13);

  
  
%% EOF

