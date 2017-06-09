
% ====================================================
%
% Make some statistical comparaison between two LZ4
% compression mode.
%
% CONUS Vincent
% 01.06.2017
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
subplot(2, 4, [1, 2]) % no compression plot

  % scaling for axis
  lz4prawScale = '';
  rawMult = 1;
  if mean(size) > 1e9
    lz4prawScale = "GB";
    rawMult = 1e9;
  elseif mean(size) > 1e6
    lz4prawScale = "MB";
    rawMult = 1e6;
  elseif mean(size) > 1e3
    lz4prawScale = "KB";
    rawMult = 1e3;
  else
    lz4prawScale = "B";
    rawMult = 1;
  end
  
  plot(size./rawMult, '-x', 'linewidth', 0.7); % plot raw size
  hold on;

  % -----------------------------------
  %axis([graphspan 7e6 8e6]);
  grid on;
  ylabel (['Raw data size [', lz4prawScale, ']'])
  xlabel 'Iterations of size measurment'

  title(["Raw ", scanTitle, " uncompressed data sizes"], 'FontWeight', 'bold', 'FontSize', 13);
  legend(sprintf("Raw data size value = %3.3f %s", size(1)/rawMult, lz4prawScale), ...
         'Location', 'southwest'
  );
  


%% =========================================================================== 
subplot(2, 4, 3) % LZ4 plot

  lz4compScale = '';
  compMult = 1;
  if mean(lz4size) > 1e9
    lz4compScale = "GB";
    compMult = 1e9;
  elseif mean(lz4size) > 1e6
    lz4compScale = "MB";
    compMult = 1e6;
  elseif mean(lz4size) > 1e3
    lz4compScale = "KB";
    compMult = 1e3;
  else
    lz4compScale = "B";
    compMult = 1;
  end
  
  plot(lz4size./compMult, '-x', 'linewidth', 0.7); % plot raw size
  hold on;

  plot(lz4graphspan, [lz4meansize./compMult lz4meansize./compMult], 'g', 'linewidth', 2); % plot mean
  plot(lz4graphspan, [lz4mediansize./compMult lz4mediansize./compMult], 'r', 'linewidth', 2); % plot median
  % standard deviation
  plot(lz4graphspan, [(lz4meansize+lz4stdsize)./compMult (lz4meansize+lz4stdsize)./compMult], 'k', 'linewidth', 2);
  plot(lz4graphspan, [(lz4meansize-lz4stdsize)./compMult (lz4meansize-lz4stdsize)./compMult], 'k', 'linewidth', 2);

  % -----------------------------------
  if (lz4stdsize/lz4meansize) > 1e-10
    axis([lz4graphspan ...
        ((lz49meansize-lz49stdsize)*(1-3*(lz4stdsize/lz4meansize)))./compMult ...
        ((lz4meansize+lz4stdsize)*(1+(lz4stdsize/lz4meansize)))./compMult] ...
        );
  end
  
  grid on;
  ylabel (['LZ4 compressed data size [', lz4compScale,']'])
  xlabel 'Iterations of compressions'

  legend(sprintf("LZ4 compressed data size value"), ...
         sprintf("Mean = %3.3f %s", lz4meansize/compMult, lz4compScale), ...
         sprintf("Median = %3.3f %s", lz4mediansize/compMult, lz4compScale), ...
         sprintf("Std = %3.3f %s (%3.3d %%)", lz4stdsize/compMult, lz4compScale, (lz4stdsize/lz4meansize)*100),...
         'Location', 'southwest'
  );
  title("LZ4 compressed data sizes", 'FontWeight', 'bold', 'FontSize', 13);


%% =========================================================================== 
subplot(2, 4, 4) % LZ4 strong plot
  plot(lz49size./compMult, '-x', 'linewidth', 0.7); % plot raw size
  hold on;

  plot(lz49graphspan, [lz49meansize./compMult lz49meansize./compMult], 'g', 'linewidth', 2); % plot mean
  plot(lz49graphspan, [lz49mediansize./compMult lz49mediansize./compMult], 'r', 'linewidth', 2); % plot median
  % standard deviation
  plot(lz49graphspan, [(lz49meansize+lz49stdsize)./compMult (lz49meansize+lz49stdsize)./compMult], 'k', 'linewidth', 2);
  plot(lz49graphspan, [(lz49meansize-lz49stdsize)./compMult (lz49meansize-lz49stdsize)./compMult], 'k', 'linewidth', 2);

  % -----------------------------------
  if (lz4stdsize/lz4meansize) > 1e-10
    axis([lz4graphspan ...
        ((lz49meansize-lz49stdsize)*(1-3*(lz4stdsize/lz4meansize)))./compMult ...
        ((lz4meansize+lz4stdsize)*(1+(lz4stdsize/lz4meansize)))./compMult] ...
        );
   end
   
      
  grid on;
  ylabel (['LZ4 compressed data size [', lz4compScale,']'])
  xlabel 'Iterations of compressions'

  legend(sprintf("LZ4 compressed data size value"), ...
         sprintf("Mean = %3.3f %s", lz49meansize/compMult, lz4compScale), ...
         sprintf("Median = %3.3f %s", lz49mediansize/compMult, lz4compScale), ...
         sprintf("Std = %3.3f %s (%3.3d %%)", lz49stdsize/compMult, lz4compScale, (lz49stdsize/lz49meansize)*100),...
         'Location', 'southwest'
  );
  title("LZ4 strong comp. data sizes", 'FontWeight', 'bold', 'FontSize', 13);

  
%% ===========================================================================
subplot(2, 4, 5) % Gain

  gain_lz4 = 1- (lz4data(:,2)./lz4data(:,1));
  gainlz4meansize = mean(gain_lz4);
  gainlz4mediansize = median(gain_lz4);
  gainlz4stdsize = std(gain_lz4);
  
  
  gain_lz49 = 1- (lz49data(:,2)./lz49data(:,1));
  gainlz49meansize = mean(gain_lz49);
  gainlz49mediansize = median(gain_lz49);
  gainlz49stdsize = std(gain_lz49);
  

  plot(gain_lz4, '-x', 'linewidth', 0.7);
  hold on;

  plot(lz4graphspan, [gainlz4meansize gainlz4meansize], 'g', 'linewidth', 2); % plot mean
  plot(lz4graphspan, [gainlz4mediansize gainlz4mediansize], 'r', 'linewidth', 2); % plot median
  % standard deviation
  plot(lz4graphspan, [gainlz4meansize+gainlz4stdsize gainlz4meansize+gainlz4stdsize], 'k', 'linewidth', 2);
  plot(lz4graphspan, [gainlz4meansize-gainlz4stdsize gainlz4meansize-gainlz4stdsize], 'k', 'linewidth', 2);

  % -----------------------------------
  if (gainlz4stdsize/gainlz4meansize)> 1e-10
    axis([lz4graphspan ...
          (gainlz4meansize-gainlz4stdsize)*(1-3*(gainlz4stdsize/gainlz4meansize)) ...
          (gainlz49meansize+gainlz49stdsize)*(1+(gainlz4stdsize/gainlz4meansize))] ...
        );
  end
      
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


  plot(gain_lz49, '-x', 'linewidth', 0.7);
  hold on;

  plot(lz49graphspan, [gainlz49meansize gainlz49meansize], 'g', 'linewidth', 2); % plot mean
  plot(lz49graphspan, [gainlz49mediansize gainlz49mediansize], 'r', 'linewidth', 2); % plot median
  % standard deviation
  plot(lz49graphspan, [gainlz49meansize+gainlz49stdsize gainlz49meansize+gainlz49stdsize], 'k', 'linewidth', 2);
  plot(lz49graphspan, [gainlz49meansize-gainlz49stdsize gainlz49meansize-gainlz49stdsize], 'k', 'linewidth', 2);

  % -----------------------------------
  if (gainlz4stdsize/gainlz4meansize) > 1e-10
    axis([lz4graphspan ...
          (gainlz4meansize-gainlz4stdsize)*(1-3*(gainlz4stdsize/gainlz4meansize)) ...
          (gainlz49meansize+gainlz49stdsize)*(1+(gainlz4stdsize/gainlz4meansize))] ...
        );
  end
  
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

  % make the right scale of the printed value and the axis
  lz4meancompScaled = '';
  lz4meandecompScaled = '';
  timeMult = '';
  
  lz4meancomp=mean(lz4data(:,3));
  lz4meandecomp=mean(lz4data(:,4));
  
  lz49meancomp=mean(lz49data(:,3));
  lz49meandecomp=mean(lz49data(:,4));
  
  
  if lz4meancomp > 1e9
    lz4meancompScaled = sprintf("%3.3d s", lz4meancomp/1e9);
    lz4meandecompScaled = sprintf("%3.3d s", lz4meandecomp/1e9);
    lz49meancompScaled = sprintf("%3.3d s", lz49meancomp/1e9);
    lz49meandecompScaled = sprintf("%3.3d s", lz49meandecomp/1e9);
    timeMult = 's';
    timeScale = 1e9;
  elseif lz4meancomp > 1e6
    lz4meancompScaled = sprintf("%3.3d ms", lz4meancomp/1e6);
    lz4meandecompScaled = sprintf("%3.3d ms", lz4meandecomp/1e6);
    lz49meancompScaled = sprintf("%3.3d ms", lz49meancomp/1e6);
    lz49meandecompScaled = sprintf("%3.3d ms", lz49meandecomp/1e6);
    timeScale = 1e6;
    timeMult = 'ms';
  elseif lz4meancomp > 1e3
    lz4meancompScaled = sprintf("%3.3d us", lz4meancomp/1000);
    lz4meandecompScaled = sprintf("%3.3d us", lz4meandecomp/1000);
    lz49meancompScaled = sprintf("%3.3d us", lz49meancomp/1000);
    lz49meandecompScaled = sprintf("%3.3d us", lz49meandecomp/1000);
    timeScale = 1e3;
    timeMult = 'us';
  else
    lz4meancompScaled = sprintf("%3.3d ns", lz4meancomp);
    lz4meandecompScaled = sprintf("%3.3d ns", lz4meandecomp);
    lz49meancompScaled = sprintf("%3.3d ns", lz49meancomp);
    lz49meandecompScaled = sprintf("%3.3d ns", lz49meandecomp);
    timeScale = 1;
    timeMult = 'ns';
  end
  

  plot(lz4data(:,3)./timeScale, '-x', 'linewidth', 0.7); % plot cmpression time
  hold on;
  plot(lz4data(:,4)./timeScale, '-xr', 'linewidth', 0.7); % plot cmpression time


  plot(lz4graphspan, [lz4meancomp./timeScale lz4meancomp./timeScale], 'c', 'linewidth', 2);
  plot(lz4graphspan, [lz4meandecomp./timeScale lz4meandecomp./timeScale], 'm', 'linewidth', 2);


  % -----------------------------------
  axis([lz4graphspan ...
        (lz4meancomp*0.6)./timeScale ...
        (lz49meancomp*1.4)./timeScale]...
      );
  grid on;
  ylabel (['Duration [', timeMult, ']'])
  xlabel 'Iterations of compressions or decompression'
  
  
  legend(sprintf("Compression"), ...
         sprintf("Decompression") , ...
         sprintf("Comp. mean = %s", lz4meancompScaled),...     
         sprintf("Deomp. mean = %s", lz4meandecompScaled),...
         'Location', 'northwest'
  );
  title ("LZ4 comp. and decomp. time", 'FontWeight', 'bold', 'FontSize', 13);

%% ===========================================================================
subplot(2, 4, 8) % Compression / decompression time for strong LZ4

  plot(lz49data(:,3)./timeScale, '-x', 'linewidth', 0.7); % plot cmpression time
  hold on;
  plot(lz49data(:,4)./timeScale, '-xr', 'linewidth', 0.7); % plot cmpression time


  plot(lz49graphspan, [lz49meancomp./timeScale lz49meancomp./timeScale], 'c', 'linewidth', 2);
  plot(lz49graphspan, [lz49meandecomp./timeScale lz49meandecomp./timeScale], 'm', 'linewidth', 2);


  % -----------------------------------
  axis([lz4graphspan ...
        (lz4meancomp*0.6)./timeScale ...
        (lz49meancomp*1.4)./timeScale]...
      );
  grid on;
  ylabel (['Duration [', timeMult, ']'])
  xlabel 'Iterations of compressions or decompression'
  
  legend(sprintf("Compression"), ...
         sprintf("Decompression") , ...
         sprintf("Comp. mean = %s", lz49meancompScaled),...     
         sprintf("Deomp. mean = %3s", lz49meandecompScaled),...
         'Location', 'northwest'
  );
  title ("LZ4 strong time", 'FontWeight', 'bold', 'FontSize', 13);

  
  
%% EOF

