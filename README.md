# DynamicSpikeDetectorPlugin

![alt text][logo]
[logo]:https://github.com/camigord/DynamicSpikeDetectorPlugin/blob/master/plugin.jpg "Plugin"

Spike detector plugin for the open-ephys [plugin-GUI](https://github.com/open-ephys/plugin-GUI/). This is a modification of the standard SpikeDetector plugin where instead of using a fixed amplitude threshold, we compute a dynamic threshold according to the signal's noise as described by [Quiroga et al.](http://www.ncbi.nlm.nih.gov/pubmed/15228749). The plugin remains compatible with the standard SpikeViewer plugin.

## What is new:

- Detection threshold is computed dynamically as described by [Quiroga et al.](http://www.ncbi.nlm.nih.gov/pubmed/15228749)
    
<p align="center">
  <b>Thr = 4&sigma;<sub>n</sub></b><br>
  <b>&sigma;<sub>n</sub> = <i>median</i>{|x| / 0.6745}</b>
</p>

- The ammount of samples which are taken before and after the spike's peak are no longer fixed, but computed based on the sampling frequency (capturing ~1ms around the peak)
- The threshold which is sent to the next module (i.e SpikeViewer) corresponds to the dynamic threshold computed at the peak of the spike.
- Some additional modifications were performed on the spike extraction code in order to avoid the extraction of spikes which are too close together (overlapping spikes), giving priority to the largest ones. Moreover, the code was improved to look for the peaks of the spikes and not only for the moment when the threshold is reached.

## Installation

Copy the SpikeDetectorDynamic folder to the plugin folder of your GUI. Then build 
the plugin as described in the [wiki](https://open-ephys.atlassian.net/wiki/display/OEW/Linux).
