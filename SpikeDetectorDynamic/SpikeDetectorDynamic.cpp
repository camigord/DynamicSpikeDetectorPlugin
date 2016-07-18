
#include <stdio.h>
#include "SpikeDetectorDynamic.h"

SpikeDetectorDynamic::SpikeDetectorDynamic()
    : GenericProcessor("Dynamic Detector"),
      overflowBuffer(2,100), dataBuffer(nullptr),
      overflowBufferSize(100), currentElectrode(-1),
	  uniqueID(0), window_size(200)
{
    //// the standard form:
    electrodeTypes.add("single electrode");
    electrodeTypes.add("stereotrode");
    electrodeTypes.add("tetrode");

    for (int i = 0; i < electrodeTypes.size()+1; i++)
    {
        electrodeCounter.add(0);
    }

	spikeBuffer.malloc(MAX_SPIKE_BUFFER_LEN);
}

SpikeDetectorDynamic::~SpikeDetectorDynamic()
{
}

AudioProcessorEditor* SpikeDetectorDynamic::createEditor()
{
	editor = new SpikeDetectorDynamicEditor(this, true);
    return editor;
}

void SpikeDetectorDynamic::updateSettings()
{
    if (getNumInputs() > 0)
        overflowBuffer.setSize(getNumInputs(), overflowBufferSize);

    for (int i = 0; i < electrodes.size(); i++)
    {
        Channel* ch = new Channel(this,i,ELECTRODE_CHANNEL);
		ch->name = generateSpikeElectrodeName(electrodes[i]->numChannels, ch->index);
		SpikeChannel* spk = new SpikeChannel(SpikeChannel::Plain, electrodes[i]->numChannels, NULL, 0);
		ch->extraData = spk;
        eventChannels.add(ch);
    }
}

bool SpikeDetectorDynamic::addElectrode(int nChans, int electrodeID)
{
    std::cout << "Adding electrode with " << nChans << " channels." << std::endl;
    int firstChan;
    if (electrodes.size() == 0)
    {
        firstChan = 0;
    }
    else
    {
        SimpleElectrode* e = electrodes.getLast();
        firstChan = *(e->channels+(e->numChannels-1))+1;
    }

    if (firstChan + nChans > getNumInputs())
    {
        firstChan = 0; // make sure we don't overflow available channels
    }

    int currentVal = electrodeCounter[nChans];
    electrodeCounter.set(nChans,++currentVal);

    String electrodeName;

    // hard-coded for tetrode configuration
    if (nChans < 3)
        electrodeName = electrodeTypes[nChans-1];
    else
        electrodeName = electrodeTypes[nChans-2];

    String newName = electrodeName.substring(0,1);
    newName = newName.toUpperCase();
    electrodeName = electrodeName.substring(1,electrodeName.length());
    newName += electrodeName;
    newName += " ";
    newName += electrodeCounter[nChans];

    SimpleElectrode* newElectrode = new SimpleElectrode();

    newElectrode->name = newName;
    newElectrode->numChannels = nChans;
	newElectrode->prePeakSamples = int(floor(0.4*getSampleRate() / 1000));			// ~1 ms around the spike as a function of sampling rate
	newElectrode->postPeakSamples = int(floor(0.7*getSampleRate() / 1000));
    newElectrode->thresholds.malloc(nChans);
    newElectrode->isActive.malloc(nChans);
    newElectrode->channels.malloc(nChans);
    newElectrode->isMonitored = false;

    for (int i = 0; i < nChans; i++)
    {
        *(newElectrode->channels+i) = firstChan+i;
        *(newElectrode->thresholds+i) = getDefaultThreshold();
        *(newElectrode->isActive+i) = true;
    }

    if (electrodeID > 0) {
        newElectrode->electrodeID = electrodeID;
        uniqueID = std::max(uniqueID, electrodeID);
    } else {
        newElectrode->electrodeID = ++uniqueID;
    }
    
    newElectrode->sourceNodeId = channels[*newElectrode->channels]->sourceNodeId;
    resetElectrode(newElectrode);
    electrodes.add(newElectrode);
    currentElectrode = electrodes.size()-1;
    return true;
}

float SpikeDetectorDynamic::getDefaultThreshold()
{
    return 4.0f;
}

StringArray SpikeDetectorDynamic::getElectrodeNames()
{
    StringArray names;

    for (int i = 0; i < electrodes.size(); i++)
    {
        names.add(electrodes[i]->name);
    }

    return names;
}

void SpikeDetectorDynamic::resetElectrode(SimpleElectrode* e)
{
    e->lastBufferIndex = 0;
}

bool SpikeDetectorDynamic::removeElectrode(int index)
{
    if (index > electrodes.size() || index < 0)
        return false;

    electrodes.remove(index);
    return true;
}

void SpikeDetectorDynamic::setElectrodeName(int index, String newName)
{
    electrodes[index-1]->name = newName;
}

void SpikeDetectorDynamic::setChannel(int electrodeIndex, int channelNum, int newChannel)
{
    std::cout << "Setting electrode " << electrodeIndex << " channel " << channelNum <<
              " to " << newChannel << std::endl;

    *(electrodes[electrodeIndex]->channels+channelNum) = newChannel;
}

int SpikeDetectorDynamic::getNumChannels(int index)
{

    if (index < electrodes.size())
        return electrodes[index]->numChannels;
    else
        return 0;
}

int SpikeDetectorDynamic::getChannel(int index, int i)
{
    return *(electrodes[index]->channels+i);
}

void SpikeDetectorDynamic::getElectrodes(Array<SimpleElectrode*>& electrodeArray)
{
	electrodeArray.addArray(electrodes);
}

SimpleElectrode* SpikeDetectorDynamic::setCurrentElectrodeIndex(int i)
{
    jassert(i >= 0 & i < electrodes.size());
    currentElectrode = i;
    return electrodes[i];
}

SimpleElectrode* SpikeDetectorDynamic::getActiveElectrode()
{
    if (electrodes.size() == 0)
        return nullptr;

    return electrodes[currentElectrode];
}

void SpikeDetectorDynamic::setChannelActive(int electrodeIndex, int subChannel, bool active)
{
    currentElectrode = electrodeIndex;
    currentChannelIndex = subChannel;

    std::cout << "Setting channel active to " << active << std::endl;

    if (active)
        setParameter(98, 1);
    else
        setParameter(98, 0);
}

bool SpikeDetectorDynamic::isChannelActive(int electrodeIndex, int i)
{
    return *(electrodes[electrodeIndex]->isActive+i);
}


void SpikeDetectorDynamic::setChannelThreshold(int electrodeNum, int channelNum, float thresh)
{
    currentElectrode = electrodeNum;
    currentChannelIndex = channelNum;
    std::cout << "Setting electrode " << electrodeNum << " channel threshold " << channelNum << " to " << thresh << std::endl;
    setParameter(99, thresh);
}

double SpikeDetectorDynamic::getChannelThreshold(int electrodeNum, int channelNum)
{
    return *(electrodes[electrodeNum]->thresholds+channelNum);
}

void SpikeDetectorDynamic::setParameter(int parameterIndex, float newValue)
{
    if (parameterIndex == 99 && currentElectrode > -1)
    {
        *(electrodes[currentElectrode]->thresholds+currentChannelIndex) = newValue;
    }
    else if (parameterIndex == 98 && currentElectrode > -1)
    {
        if (newValue == 0.0f)
            *(electrodes[currentElectrode]->isActive+currentChannelIndex) = false;
        else
            *(electrodes[currentElectrode]->isActive+currentChannelIndex) = true;
    }
}

bool SpikeDetectorDynamic::enable()
{
    sampleRateForElectrode = (uint16_t) getSampleRate();
    useOverflowBuffer.clear();

    for (int i = 0; i < electrodes.size(); i++)
        useOverflowBuffer.add(false);

    return true;
}

bool SpikeDetectorDynamic::disable()
{
    for (int n = 0; n < electrodes.size(); n++)
    {
        resetElectrode(electrodes[n]);
    }

    return true;
}

void SpikeDetectorDynamic::addSpikeEvent(SpikeObject* s, MidiBuffer& eventBuffer, int peakIndex)
{
    s->eventType = SPIKE_EVENT_CODE;

    int numBytes = packSpike(s,                        // SpikeObject
                             spikeBuffer,              // uint8_t*
                             MAX_SPIKE_BUFFER_LEN);    // int

    if (numBytes > 0)
        eventBuffer.addEvent(spikeBuffer, numBytes, peakIndex);
}

void SpikeDetectorDynamic::addWaveformToSpikeObject(SpikeObject* s,
                                             int& peakIndex,
                                             int& electrodeNumber,
                                             int& currentChannel,
											 int dyn_threshold)
{
    int spikeLength = electrodes[electrodeNumber]->prePeakSamples + electrodes[electrodeNumber]->postPeakSamples;
    s->timestamp = getTimestamp(currentChannel) + peakIndex;
    s->nSamples = spikeLength;

    int chan = *(electrodes[electrodeNumber]->channels+currentChannel);

    s->gain[currentChannel] = (int)(1.0f / channels[chan]->bitVolts)*1000;
	s->threshold[currentChannel] = dyn_threshold;

    if (isChannelActive(electrodeNumber, currentChannel))
    {
        for (int sample = 0; sample < spikeLength; sample++)
        {
            // warning -- be careful of bitvolts conversion
            s->data[currentIndex] = uint16(getNextSample(*(electrodes[electrodeNumber]->channels+currentChannel)) / channels[chan]->bitVolts + 32768);

            currentIndex++;
            sampleIndex++;
        }
    }
    else
    {
        for (int sample = 0; sample < spikeLength; sample++)
        {
            // insert a blank spike if the
            s->data[currentIndex] = 0;
            currentIndex++;
            sampleIndex++;
        }
    }
    sampleIndex -= spikeLength; // reset sample index
}

void SpikeDetectorDynamic::handleEvent(int eventType, MidiMessage& event, int sampleNum)
{
    if (eventType == TIMESTAMP)
    {
        const uint8* dataptr = event.getRawData();
        memcpy(&timestamp, dataptr + 4, 8); // remember to skip first four bytes
    }
}

void SpikeDetectorDynamic::process(AudioSampleBuffer& buffer,MidiBuffer& events)
{
    // cycle through electrodes
    SimpleElectrode* electrode;
    dataBuffer = &buffer;
    checkForEvents(events); // need to find any timestamp events before extracting spikes

    for (int i = 0; i < electrodes.size(); i++)
    {
        electrode = electrodes[i];

        // refresh buffer index for this electrode
        sampleIndex = electrode->lastBufferIndex - 1; // subtract 1 to account for
        // increment at start of getNextSample()

        int nSamples = getNumSamples(*electrode->channels);

		// Compute dynamic thresholds
		const int number_of_windows = (int)ceil((float)((nSamples + (overflowBufferSize / 2))) / window_size);
		std::vector<std::vector<float> > dyn_thresholds;
		dyn_thresholds.resize(electrode->numChannels);
		for (int i = 0; i < electrode->numChannels; ++i)
			dyn_thresholds[i].resize(number_of_windows);

		int window_number = 0;
		int sample_counter = 0;
		for (int chan = 0; chan < electrode->numChannels; chan++)
		{
			int currentChannel = *(electrode->channels + chan);
			std::vector<float> temp_values(window_size);

			while (samplesAvailable(nSamples))
			{
				sampleIndex++;
				temp_values[sample_counter] = abs(getNextSample(currentChannel)) / scalar;
				if (sample_counter == window_size - 1)
				{
					// Compute Threshold using values in 'temp_values'
					std::sort(temp_values.begin(), temp_values.end());
					float Threshold = float(*(electrode->thresholds + chan));
					dyn_thresholds[chan][window_number] = Threshold * temp_values[floor((float)temp_values.size() / 2)];
					window_number++;
					sample_counter = 0;
				}
				else
				{
					sample_counter++;
				}
			}
			// Check last window
			if (sample_counter != 0)
			{
				// Remove empty elements from 'temp_values'
				temp_values.erase(temp_values.begin() + sample_counter, temp_values.end());
				// Compute Threshold using values in 'temp_values'
				std::sort(temp_values.begin(), temp_values.end());
				float Threshold = float(*(electrode->thresholds + chan));
				dyn_thresholds[chan][window_number] = Threshold * temp_values[floor((float)temp_values.size() / 2)];
			}

			// Restart indexes
			sampleIndex = electrode->lastBufferIndex - 1;
			window_number = 0;
			sample_counter = 0;
		}

        // cycle through samples
        while (samplesAvailable(nSamples))
        {
            sampleIndex++;
			// Check in which window is the sample located
			if (sample_counter == window_size - 1)
			{
				window_number++;
				sample_counter = 0;
			}
			else
			{
				sample_counter++;
			}
            // cycle through channels
            for (int chan = 0; chan < electrode->numChannels; chan++)
            {
                if (*(electrode->isActive+chan))
                {
                    int currentChannel = *(electrode->channels+chan);

					if (abs(getNextSample(currentChannel)) > dyn_thresholds[chan][window_number]) // trigger spike
                    {
                        // find the peak
                        int peakIndex = sampleIndex;
						sampleIndex++;
						while (abs(getCurrentSample(currentChannel)) < abs(getNextSample(currentChannel)))
						{
							sampleIndex++;		// Keep going until finding the largest point or peak
						}
						peakIndex = sampleIndex - 1;
						float peak_amp = abs(getCurrentSample(currentChannel));

						// check that there are no other peaks happening within num_samples (prePeakSamples + postPeakSamples)
						int num_samples = electrode->prePeakSamples + electrode->postPeakSamples;
						int current_test_sample = 1;
						while (current_test_sample < num_samples)
						{
							if (peak_amp > abs(getNextSample(currentChannel)))
							{
								current_test_sample++;
								sampleIndex++;
							}
							else
							{
								peakIndex = sampleIndex;
								peak_amp = abs(getCurrentSample(currentChannel));
								sampleIndex++;
								current_test_sample = 1;
							}
						}

						sampleIndex = peakIndex - (electrode->prePeakSamples - 1);

                        SpikeObject newSpike;
                        newSpike.timestamp = 0; //getTimestamp(currentChannel) + peakIndex;
                        newSpike.timestamp_software = -1;
                        newSpike.source = i;
                        newSpike.nChannels = electrode->numChannels;
                        newSpike.sortedId = 0;
                        newSpike.electrodeID = electrode->electrodeID;
                        newSpike.channel = 0;
                        newSpike.samplingFrequencyHz = sampleRateForElectrode;

                        currentIndex = 0;

                        // package spikes;
                        for (int channel = 0; channel < electrode->numChannels; channel++)
                        {
							addWaveformToSpikeObject(&newSpike, peakIndex, i, channel, int(floor(dyn_thresholds[chan][window_number])));
                        }
                        addSpikeEvent(&newSpike, events, peakIndex);

                        // advance the sample index
                        sampleIndex = peakIndex + electrode->postPeakSamples;
                        break; // quit spike "for" loop
                    } // end spike trigger

                } // end if channel is active
            } // end cycle through channels on electrode

        } // end cycle through samples

        electrode->lastBufferIndex = sampleIndex - nSamples; // should be negative

        if (nSamples > overflowBufferSize)
        {
            for (int j = 0; j < electrode->numChannels; j++)
            {
				overflowBuffer.copyFrom(*electrode->channels+j, 0,buffer, *electrode->channels+j,nSamples-overflowBufferSize,overflowBufferSize);
            }
            useOverflowBuffer.set(i, true);
        }
        else
        {
            useOverflowBuffer.set(i, false);
        }

    } // end cycle through electrodes
}

float SpikeDetectorDynamic::getNextSample(int& chan)
{
    if (sampleIndex < 0)
    {
        int ind = overflowBufferSize + sampleIndex;

        if (ind < overflowBuffer.getNumSamples())
            return *overflowBuffer.getWritePointer(chan, ind);
        else
            return 0;
    }
    else
    {
        if (sampleIndex < dataBuffer->getNumSamples())
            return *dataBuffer->getWritePointer(chan, sampleIndex);
        else
            return 0;
    }
}

float SpikeDetectorDynamic::getCurrentSample(int& chan)
{
    if (sampleIndex < 1)
    {
        return *overflowBuffer.getWritePointer(chan, overflowBufferSize + sampleIndex - 1);
    }
    else
    {
        return *dataBuffer->getWritePointer(chan, sampleIndex - 1);
    }
}

bool SpikeDetectorDynamic::samplesAvailable(int nSamples)
{
    if (sampleIndex > nSamples - overflowBufferSize/2)
    {
        return false;
    }
    else
    {
        return true;
    }
}

void SpikeDetectorDynamic::saveCustomParametersToXml(XmlElement* parentElement)
{
    for (int i = 0; i < electrodes.size(); i++)
    {
        XmlElement* electrodeNode = parentElement->createNewChildElement("ELECTRODE");
        electrodeNode->setAttribute("name", electrodes[i]->name);
        electrodeNode->setAttribute("numChannels", electrodes[i]->numChannels);
        electrodeNode->setAttribute("prePeakSamples", electrodes[i]->prePeakSamples);
        electrodeNode->setAttribute("postPeakSamples", electrodes[i]->postPeakSamples);
        electrodeNode->setAttribute("electrodeID", electrodes[i]->electrodeID);

        for (int j = 0; j < electrodes[i]->numChannels; j++)
        {
            XmlElement* channelNode = electrodeNode->createNewChildElement("SUBCHANNEL");
            channelNode->setAttribute("ch",*(electrodes[i]->channels+j));
            channelNode->setAttribute("thresh",*(electrodes[i]->thresholds+j));
            channelNode->setAttribute("isActive",*(electrodes[i]->isActive+j));
        }
    }
}

void SpikeDetectorDynamic::loadCustomParametersFromXml()
{
    if (parametersAsXml != nullptr) // prevent double-loading
    {
        // use parametersAsXml to restore state
		SpikeDetectorDynamicEditor* sde = (SpikeDetectorDynamicEditor*)getEditor();

        int electrodeIndex = -1;
        forEachXmlChildElement(*parametersAsXml, xmlNode)
        {
            if (xmlNode->hasTagName("ELECTRODE"))
            {
                electrodeIndex++;
                std::cout << "ELECTRODE>>>" << std::endl;

                int channelsPerElectrode = xmlNode->getIntAttribute("numChannels");
                int electrodeID = xmlNode->getIntAttribute("electrodeID");

                sde->addElectrode(channelsPerElectrode, electrodeID);

                setElectrodeName(electrodeIndex+1, xmlNode->getStringAttribute("name"));
                sde->refreshElectrodeList();

                int channelIndex = -1;

                forEachXmlChildElement(*xmlNode, channelNode)
                {
                    if (channelNode->hasTagName("SUBCHANNEL"))
                    {
                        channelIndex++;

                        std::cout << "Subchannel " << channelIndex << std::endl;

                        setChannel(electrodeIndex, channelIndex, channelNode->getIntAttribute("ch"));
                        setChannelThreshold(electrodeIndex, channelIndex, channelNode->getDoubleAttribute("thresh"));
                        setChannelActive(electrodeIndex, channelIndex, channelNode->getBoolAttribute("isActive"));
                    }
                }
            }
        }
        sde->checkSettings();
    }
}

