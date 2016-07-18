
#ifndef __SPIKEDETECTORDYNAMICEDITOR_H_F0BD2DD9__
#define __SPIKEDETECTORDYNAMICEDITOR_H_F0BD2DD9__

#include <EditorHeaders.h>

class TriangleButton;
class UtilityButton;

/**
  User interface for the SpikeDetector using dynamic thresholds.

  Allows the user to add single electrodes, stereotrodes, or tetrodes.

  Parameters of individual channels, such as channel mapping, threshold,
  and enabled state, can be edited.
*/

class SpikeDetectorDynamicEditor : public GenericEditor,
    public Label::Listener,
    public ComboBox::Listener

{
public:
	SpikeDetectorDynamicEditor(GenericProcessor* parentNode, bool useDefaultParameterEditors);
	virtual ~SpikeDetectorDynamicEditor();
    void buttonEvent(Button* button);
    void labelTextChanged(Label* label);
    void comboBoxChanged(ComboBox* comboBox);
    void sliderEvent(Slider* slider);
    void channelChanged (int channel, bool newState) override;
    bool addElectrode(int nChans, int electrodeID = 0);
    void removeElectrode(int index);
    void checkSettings();
    void refreshElectrodeList();

private:

    void drawElectrodeButtons(int);

    ComboBox* electrodeTypes;
    ComboBox* electrodeList;
    Label* numElectrodes;
    Label* thresholdLabel;
    TriangleButton* upButton;
    TriangleButton* downButton;
    UtilityButton* plusButton;

    ThresholdSlider* thresholdSlider;

    OwnedArray<ElectrodeButton> electrodeButtons;
    Array<ElectrodeEditorButton*> electrodeEditorButtons;

    void editElectrode(int index, int chan, int newChan);

    int lastId;
    bool isPlural;

    Font font;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpikeDetectorDynamicEditor);
};
#endif  // __SPIKEDETECTORDYNAMICEDITOR_H_F0BD2DD9__
