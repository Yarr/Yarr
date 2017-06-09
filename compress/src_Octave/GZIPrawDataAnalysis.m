
% ====================================================
%
% Make some statistical analysis with GZIP and ZCAT
%
% CONUS Vincent
% 06.06.2017
%
% ====================================================   


% ###################################################################

% CHANGE VALUE HERE FOR DIFFERENT TYPE OF SCAN

% ###################################################################
% put the correct value here to have the right form for the graphic 
% Totscan       : 1
% Analogscan    : 2
% Digitalscan   : 3
% Thresholdscan : 4

scanMode = 4;

% ###################################################################

scanTitle = '';
switch scanMode
  case 1
    scanTitle = 'Totscan';
  case 2
    scanTitle = 'Analogscan';
  case 3
    scanTitle = 'Digitalscan';
  case 4
    scanTitle = 'Thresholdscan';
  otherwise 
    scanTitle = 'PLEASE ENTER A CORRECT VALUE';
end

pkg load statistics % package useful for boxplot and histograms

measures


%% ===========================================================================
%% ===========================================================================
figure('Position', [20 20 1300 740]); % 720p window
%% ===========================================================================
subplot(2, 2, 1) % no compression plot

  % scaling for axis
  gziprawScale = '';
  rawMult = 1;
  if mean(gzipraw) > 1e9
    gziprawScale = "GB";
    rawMult = 1e9;
  elseif mean(gzipraw) > 1e6
    gziprawScale = "MB";
    rawMult = 1e6;
  elseif mean(gzipraw) > 1e3
    gziprawScale = "KB";
    rawMult = 1e3;
  else
    gziprawScale = "B";
    rawMult = 1;
  end

  plot(gzipraw./rawMult, '-x', 'linewidth', 0.7); % plot raw size
  hold on;

  % -----------------------------------
  %axis([graphspan 7e6 8e6]);
  grid on;
  ylabel (['Raw data size [', gziprawScale,']'])
  xlabel 'Iterations of size measurment'

  title(["Raw ", scanTitle, " uncompressed data sizes"], 'FontWeight', 'bold', 'FontSize', 13);
  legend(sprintf("Raw data size value  = %3.3f %s", gzipraw(1)/rawMult, gziprawScale), ...
         'Location', 'southwest'
  );
  
subplot(2, 2, 2) % GZIP compression 

  gzipcompScale = '';
  compMult = 1;
  if mean(gzipsize) > 1e9
    gzipcompScale = "GB";
    compMult = 1e9;
  elseif mean(gzipsize) > 1e6
    gzipcompScale = "MB";
    compMult = 1e6;
  elseif mean(gzipsize) > 1e3
    gzipcompScale = "KB";
    compMult = 1e3;
  else
    gzipcompScale = "B";
    compMult = 1;
  end

  plot(gzipsize./compMult, '-x', 'linewidth', 0.7); % plot raw size
  hold on;

  plot(gzipgraphspan, [gzipmeansize./compMult gzipmeansize./compMult], 'g', 'linewidth', 2); % plot mean
  plot(gzipgraphspan, [gzipmediansize./compMult gzipmediansize./compMult], 'r', 'linewidth', 2); % plot median
  % standard deviation
  plot(gzipgraphspan, [(gzipmeansize+gzipstdsize)./compMult (gzipmeansize+gzipstdsize)./compMult], 'k', 'linewidth', 2);
  plot(gzipgraphspan, [(gzipmeansize-gzipstdsize)./compMult (gzipmeansize-gzipstdsize)./compMult], 'k', 'linewidth', 2);


  % -----------------------------------
  if (gzipstdsize/gzipmeansize) != 0
    axis([gzipgraphspan ...
          ((gzipmeansize-gzipstdsize)*(1-2*(gzipstdsize/gzipmeansize)))./compMult ...
          ((gzipmeansize+gzipstdsize)*(1+(gzipstdsize/gzipmeansize)))./compMult] ...
        );
  end
  
  grid on;
  ylabel (['Raw data size [', gzipcompScale, ']'])
  xlabel 'Iterations of size measurment'

  title("GZIP compressed data size value", 'FontWeight', 'bold', 'FontSize', 13);
  legend(sprintf("GZIP compressed data size value"), ...
         sprintf("Mean = %3.2f %s", gzipmeansize/compMult, gzipcompScale), ...
         sprintf("Median = %3.2f %s", gzipmediansize/compMult, gzipcompScale), ...
         sprintf("Standard deviation = %3.2f %s (%3.3d %%)", gzipstdsize/compMult, gzipcompScale, (gzipstdsize/gzipmeansize)*100),...
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
  if ((gaingzipstdsize/gaingzipmeansize) > 1e-10)
    axis([gzipgraphspan ...
         (gaingzipmeansize-gaingzipstdsize)*(1-2*(gaingzipstdsize/gaingzipmeansize)) ...
         (gaingzipmeansize+gaingzipstdsize)*(1+(gaingzipstdsize/gaingzipmeansize))] ...
        );
  end
  
  grid on;
  ylabel 'Compression gain'
  xlabel 'Iterations of compressions'
  set(gca, 'YTickMode','manual')
  set(gca, 'YTickLabel',num2str(100.*get(gca,'YTick')','%g%%'))
  legend(sprintf("GZIP compression gain"), ...
         sprintf("Mean = %d %%", gaingzipmeansize*100), ...
         sprintf("Median = %d %%", gaingzipmediansize*100), ...
         sprintf("Std = %d %% (%3.3d %%)", gaingzipstdsize*100, (gaingzipstdsize/gaingzipmeansize)*100),...
         'Location', 'southwest'
  );
  title ("GZIP compression gain", 'FontWeight', 'bold', 'FontSize', 13);
  
  
subplot(2, 2, 4) % Compression / decompression time

  % make the right scale of the printed value and the axis
  gzipmeancompScaled = '';
  gzipmeandecompScaled = '';
  timeMult = '';
  if gzipmeancomp > 1e9
    gzipmeancompScaled = sprintf("%3.3d s", gzipmeancomp/1e9);
    gzipmeandecompScaled = sprintf("%3.3d s", gzipmeandecomp/1e9);
    timeMult = 's';
    timeScale = 1e9;
  elseif gzipmeancomp > 1e6
    gzipmeancompScaled = sprintf("%3.3d ms", gzipmeancomp/1e6);
    gzipmeandecompScaled = sprintf("%3.3d ms", gzipmeandecomp/1e6);
    timeScale = 1e6;
    timeMult = 'ms';
  elseif gzipmeancomp > 1e3
    gzipmeancompScaled = sprintf("%3.3d us", gzipmeancomp/1000);
    gzipmeandecompScaled = sprintf("%3.3d us", gzipmeandecomp/1000);
    timeScale = 1e3;
    timeMult = 'us';
  else
    gzipmeancompScaled = sprintf("%3.3d ns", gzipmeancomp);
    gzipmeandecompScaled = sprintf("%3.3d ns", gzipmeandecomp);
    timeScale = 1;
    timeMult = 'ns';
  end
  

  plot(gzipdata(:,3)./timeScale, '-x', 'linewidth', 0.7); % plot cmpression time
  hold on;
  plot(gzipdata(:,4)./timeScale, '-xr', 'linewidth', 0.7); % plot cmpression time

  gzipmeancomp=mean(gzipdata(:,3));
  gzipmeandecomp=mean(gzipdata(:,4));

  plot(gzipgraphspan, [gzipmeancomp./timeScale gzipmeancomp./timeScale], 'c', 'linewidth', 2);
  plot(gzipgraphspan, [gzipmeandecomp./timeScale gzipmeandecomp./timeScale], 'm', 'linewidth', 2);


  % -----------------------------------
  axis([gzipgraphspan (gzipmeandecomp*0.6)./timeScale (gzipmeancomp*1.4)./timeScale]);
  grid on;
  ylabel (['Duration [', timeMult, ']'])
  xlabel 'Iterations of compressions or decompression'
  

  legend(sprintf("Compression"), ...
         sprintf("Decompression") , ...
         sprintf("Comp. mean = %s", gzipmeancompScaled),...     
         sprintf("Deomp. mean = %s", gzipmeandecompScaled),...
         'Location', 'northwest'
  );
  title ("GZIP comp. and decomp. time", 'FontWeight', 'bold', 'FontSize', 13);
