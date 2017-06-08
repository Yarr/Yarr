
% ====================================================
%
% Make some statistical analysis with GZIP and ZCAT
%
% CONUS Vincent
% 06.06.2017
%
% ====================================================   

pkg load statistics % package useful for boxplot and histograms

measures


%% ===========================================================================
%% ===========================================================================
figure('Position', [20 20 1300 740]); % 720p window
%% ===========================================================================
subplot(2, 2, 1) % no compression plot

  plot(gzipraw, '-x', 'linewidth', 0.7); % plot raw size
  hold on;

  % -----------------------------------
  %axis([graphspan 7e6 8e6]);
  grid on;
  ylabel 'Raw data size [Byte]'
  xlabel 'Iterations of size measurment'

  title("Raw ThresholdScan uncompressed data sizes", 'FontWeight', 'bold', 'FontSize', 13);
  legend(sprintf("Raw data size value  = %3.3f MB", gzipraw(1)/1000000), ...
         %sprintf("Mean = %3.2f MB", meansize/1000000), ...
         %sprintf("Median = %3.2f MB", mediansize/1000000), ...
         %sprintf("Standard deviation = %3.2f MB (%3.3d %%)", stdsize/1000000, (stdsize/meansize)*100),...
         'Location', 'southwest'
  );
  
subplot(2, 2, 2) % GZIP compression 

  plot(gzipsize, '-x', 'linewidth', 0.7); % plot raw size
  hold on;

  plot(gzipgraphspan, [gzipmeansize gzipmeansize], 'g', 'linewidth', 2); % plot mean
  plot(gzipgraphspan, [gzipmediansize gzipmediansize], 'r', 'linewidth', 2); % plot median
  % standard deviation
  plot(gzipgraphspan, [gzipmeansize+gzipstdsize gzipmeansize+gzipstdsize], 'k', 'linewidth', 2);
  plot(gzipgraphspan, [gzipmeansize-gzipstdsize gzipmeansize-gzipstdsize], 'k', 'linewidth', 2);


  % -----------------------------------
  %axis([graphspan 7e6 8e6]);
  grid on;
  ylabel 'Raw data size [Byte]'
  xlabel 'Iterations of size measurment'

  title("GZIP compressed data size value", 'FontWeight', 'bold', 'FontSize', 13);
  legend(sprintf("GZIP compressed data size value"), ...
         sprintf("Mean = %3.2f KB", gzipmeansize/1000), ...
         sprintf("Median = %3.2f KB", gzipmediansize/1000), ...
         sprintf("Standard deviation = %3.2f MB (%3.3d %%)", gzipstdsize/1000000, (gzipstdsize/gzipmeansize)*100),...
         'Location', 'southwest'
  );
  
subplot(2, 2, 3) % Gain

  gain_gzip = 1- (gzipdata(:,2)./gzipdata(:,1));
  gaingzipmeansize = mean(gain_gzip);
  gaingzipmediansize = median(gain_gzip);
  gaingzipstdsize = std(gain_gzip);

  plot(gain_gzip, '-x', 'linewidth', 0.7);
  hold on;

  plot(gzipgraphspan, [gaingzipmeansize gaingzipmeansize], 'g', 'linewidth', 2); % plot mean
  plot(gzipgraphspan, [gaingzipmediansize gaingzipmediansize], 'r', 'linewidth', 2); % plot median
  % standard deviation
  plot(gzipgraphspan, [gaingzipmeansize+gaingzipstdsize gaingzipmeansize+gaingzipstdsize], 'k', 'linewidth', 2);
  plot(gzipgraphspan, [gaingzipmeansize-gaingzipstdsize gaingzipmeansize-gaingzipstdsize], 'k', 'linewidth', 2);

  % -----------------------------------
  %axis([lz4graphspan 0.983 0.9855]);
  grid on;
  ylabel 'Compression gain'
  xlabel 'Iterations of compressions'
  set(gca, 'YTickMode','manual')
  set(gca, 'YTickLabel',num2str(100.*get(gca,'YTick')','%g%%'))
  legend(sprintf("LZ4 compression gain"), ...
         sprintf("Mean = %d %%", gaingzipmeansize*100), ...
         sprintf("Median = %d %%", gaingzipmediansize*100), ...
         sprintf("Std = %d %% (%3.3d %%)", gaingzipstdsize*100, (gaingzipstdsize/gaingzipmeansize)*100),...
         'Location', 'southwest'
  );
  title ("GZIP compression gain", 'FontWeight', 'bold', 'FontSize', 13);
  
  
subplot(2, 2, 4) % Compression / decompression time

  plot(gzipdata(:,3), '-x', 'linewidth', 0.7); % plot cmpression time
  hold on;
  plot(gzipdata(:,4), '-xr', 'linewidth', 0.7); % plot cmpression time

  gzipmeancomp=mean(gzipdata(:,3));
  gzipmeandecomp=mean(gzipdata(:,4));

  plot(gzipgraphspan, [gzipmeancomp gzipmeancomp], 'c', 'linewidth', 2);
  plot(gzipgraphspan, [gzipmeandecomp gzipmeandecomp], 'm', 'linewidth', 2);


  % -----------------------------------
  %axis([gzipgraphspan 9e6 6.5e7]);
  grid on;
  ylabel 'Duration [ns]'
  xlabel 'Iterations of compressions or decompression'
  legend(sprintf("Compression"), ...
         sprintf("Decompression") , ...
         sprintf("Comp. mean = %3.3d us", gzipmeancomp/1000000),...     
         sprintf("Deomp. mean = %3.3d us", gzipmeandecomp/1000000),...
         'Location', 'northwest'
  );
  title ("GZIP comp. and decomp. time", 'FontWeight', 'bold', 'FontSize', 13);
