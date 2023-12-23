#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

#include <numbers>
#include <limits>
#include <memory>
#include <fstream>
#include <random>

#include "InterfaceDefines.h"

//#include "juce_opengl/utils/juce_OpenGLAppComponent.h"
//https://medium.com/@Im_Jimmi/using-opengl-for-2d-graphics-in-a-juce-plug-in-24aa82f634ff
//https://docs.juce.com/master/tutorial_open_gl_application.html


class AudioDevicePickTable : public juce::Component,
                             public juce::TableListBoxModel
{
public:
    AudioDevicePickTable()
    {
        addAndMakeVisible(&table);
        table.setModel(this);

        table.getHeader().addColumn("Using", 1, 1, 30, -1, juce::TableHeaderComponent::ColumnPropertyFlags::notResizableOrSortable);
        table.getHeader().addColumn("Microphone Devices", 2, 1, 30, -1, juce::TableHeaderComponent::ColumnPropertyFlags::notResizableOrSortable);
        table.getHeader().addColumn("Fix mono", 3, 1, 30, -1, juce::TableHeaderComponent::ColumnPropertyFlags::notResizableOrSortable);

        table.getHeader().setPopupMenuActive(false);
    }
    ~AudioDevicePickTable()
    {

    }

    void paintRowBackground(juce::Graphics& g, int rowNumber, int width, int height, bool rowIsSelected) override
    {
        if (rowIsSelected)
            g.fillAll(juce::Colours::lightblue);
    }

    void paintCell(juce::Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected) override
    {
        g.setColour(juce::Colours::bisque);
        if (columnId == 2)
        {
            if (rowNumber < MicrophoneDevices.size())
            {
                g.drawText(juce::CharPointer_UTF16(MicrophoneDevices[rowNumber].GetAudioDeviceName().c_str()), 2, 0, width, height,
                    juce::Justification::centredLeft, true);
            }
            else
            {
                SafePointerDereference(Singleton<VariableMainLogger>::GetInstance().GetTracePtr(), P7_CRITICAL(0, TM("[LOGIC ERROR] rowNumber >= MicrophoneDevices.size()")));
            }
        }
    }

    void resized() override
    {
        table.setBoundsInset(juce::BorderSize<int>(getWidth() * 0.025));

        table.autoSizeAllColumns();

        table.resized();
    }

    void paint(juce::Graphics& g) override
    {

    }

    int getNumRows() override
    {
        return MicrophoneDevices.size();
    }

    int getColumnAutoSizeWidth(int columnId) override
    {
        const int Width = table.getWidth();

        switch (columnId)
        {
        case 1:
            if (std::round(Width * 0.1) > Width * 0.1 &&
                std::round(Width * 0.75) > Width * 0.75 &&
                std::round(Width * 0.15) > Width * 0.15)
            {
                return Width * 0.1;
            }
            else
            {
                return std::round(Width * 0.1);
            }

        case 2:
            return std::round(Width * 0.75);

        case 3:
            return std::round(Width * 0.15);

        default:
            SafePointerDereference(Singleton<VariableMainLogger>::GetInstance().GetTracePtr(), P7_CRITICAL(0, TM("[LOGIC IMPLEMENTATION ERROR] Need Implement size for new columnId = %d"),
                columnId));
            return 0;
        }
    }


    void update()
    {
        MicrophoneDevices = GetAllMicrohponeDevices();

        IsApplyChangesButtonActive = false;
        for (const auto& value : MicrophoneDevices)
        {
            IsApplyChangesButtonActive |= value.GetNeedUpdate();
        }

        if (m_pApplyChangesButton)
        {
            m_pApplyChangesButton->setEnabled(IsApplyChangesButtonActive);
        }
    }

    Component* refreshComponentForCell(int rowNumber, int columnId, bool isRowSelected,
        Component* existingComponentToUpdate) override
    {
        if (columnId == 1)
        {
            auto* SelectionBox = static_cast<SelectionAudioDeviceColumnCustomComponent*> (existingComponentToUpdate);
            if (SelectionBox == nullptr)
            {
                SelectionBox = new SelectionAudioDeviceColumnCustomComponent(*this);
            }

            SelectionBox->setRowAndColumn(rowNumber, columnId);
            return SelectionBox;
        }
        else if (columnId == 3)
        {
            auto* SelectionBox = static_cast<SelectionFixMonoColumnCustomComponent*> (existingComponentToUpdate);
            if (SelectionBox == nullptr)
            {
                SelectionBox = new SelectionFixMonoColumnCustomComponent(*this);
            }

            SelectionBox->setRowAndColumn(rowNumber, columnId);
            return SelectionBox;
        }

        jassert(existingComponentToUpdate == nullptr);
        return nullptr;
    }

private:
    class SelectionAudioDeviceColumnCustomComponent : public Component
    {
    public:
        SelectionAudioDeviceColumnCustomComponent(AudioDevicePickTable& td)
            : owner(td)
        {
            addAndMakeVisible(toggleButton);

            toggleButton.onClick = [this]
            {
                owner.IsApplyChangesButtonActive = true;

                owner.MicrophoneDevices[row].InvertNeedUpdate();
                owner.MicrophoneDevices[row].SetNeedInstalling(GetToggleState());   //Install or Uninstall == i / u

                owner.m_pApplyChangesButton->setEnabled(owner.IsApplyChangesButtonActive);
            };
        }

        void resized() override
        {
            auto area = getLocalBounds();
            area.removeFromLeft(area.getWidth() / 8 + 2);
            area.removeFromTop(2);

            toggleButton.setBounds(area);
        }

        void setRowAndColumn(int newRow, int newColumn)
        {
            row = newRow;
            columnId = newColumn;

            //Init toggleButton state
            toggleButton.setToggleState(owner.MicrophoneDevices[row].GetNeedInstalling(), juce::dontSendNotification);
        }

        bool GetToggleState()
        {
            return toggleButton.getToggleState();
        }

    private:
        AudioDevicePickTable& owner;
        juce::ToggleButton toggleButton;
        int row, columnId;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SelectionAudioDeviceColumnCustomComponent)
    };

    friend class SelectionAudioDeviceColumnCustomComponent;



    class SelectionFixMonoColumnCustomComponent : public Component
    {
    public:
        SelectionFixMonoColumnCustomComponent(AudioDevicePickTable& td)
            : owner(td)
        {
            addAndMakeVisible(toggleButton);

            toggleButton.onClick = [this]
            {
                ManageAPO(ManageAPOParams::SetFixMonoValue, owner.MicrophoneDevices[row].GetDeviceGUID());

                toggleButton.setToggleState(GetFixMonoSoundValue(), juce::dontSendNotification);
            };
        }

        void resized() override
        {
            auto area = getLocalBounds();
            area.removeFromLeft(area.getWidth() / 4 + 2);
            area.removeFromTop(2);

            toggleButton.setBounds(area);
        }

        void setRowAndColumn(int newRow, int newColumn)
        {
            row = newRow;
            columnId = newColumn;

            toggleButton.setToggleState(GetFixMonoSoundValue(), juce::dontSendNotification);
        }

        bool GetToggleState()
        {
            return toggleButton.getToggleState();
        }

        DWORD GetFixMonoSoundValue()
        {
            const std::wstring BaseSoftwarePathGUID = std::format(L"SOFTWARE\\{}\\{}", APOName, owner.MicrophoneDevices[row].GetDeviceGUID());
            winreg::RegKey BaseSoftwareKey;
            if (auto BaseSoftwareKeyValue = BaseSoftwareKey.TryOpen(HKEY_LOCAL_MACHINE, BaseSoftwarePathGUID, GENERIC_READ | KEY_QUERY_VALUE | KEY_WOW64_64KEY);
                BaseSoftwareKeyValue.IsOk())
            {
                DWORD FixMonoSound;
                if (const auto Code = GetValueFunction(BaseSoftwareKey, L"FixMonoSound", FixMonoSound);
                    Code == ERROR_SUCCESS)
                {
                    return FixMonoSound;
                }
                else
                {
                    SafePointerDereference(Singleton<VariableMainLogger>::GetInstance().GetTracePtr(), P7_TRACE(0, TM("GetValueFunction(%s, FixMonoSound) error: %s"),
                        BaseSoftwarePathGUID.c_str(), ParseWindowsError(Code).c_str()));

                    return 0;     //false
                }
            }
            else
            {
                SafePointerDereference(Singleton<VariableMainLogger>::GetInstance().GetTracePtr(), P7_TRACE(0, TM("TryOpen(%s, GENERIC_READ | KEY_QUERY_VALUE) error: %s"),
                    BaseSoftwarePathGUID.c_str(), ParseWindowsError(BaseSoftwareKeyValue.Code()).c_str()));

                return 0;     //false
            }

            SafePointerDereference(Singleton<VariableMainLogger>::GetInstance().GetTracePtr(), P7_CRITICAL(0, TM("[REALISATION ERROR] Dead code reached")));
            return 0;     //false
        }

        //DWORD FixMonoSound(bool IsGettingCurrentState)
        //{
        //    const std::wstring BaseSoftwarePathGUID = std::format(L"SOFTWARE\\{}\\{}", APOName, owner.MicrophoneDevices[row].GetDeviceGUID());
        //    winreg::RegKey BaseSoftwareKey;
        //    if (auto BaseSoftwareKeyValue = BaseSoftwareKey.TryOpen(HKEY_LOCAL_MACHINE, BaseSoftwarePathGUID, GENERIC_READ | KEY_QUERY_VALUE | KEY_WOW64_64KEY);
        //        BaseSoftwareKeyValue.IsOk())
        //    {
        //        DWORD FixMonoSound;
        //        if (const auto Code = GetValueFunction(BaseSoftwareKey, L"FixMonoSound", FixMonoSound);
        //            Code == ERROR_SUCCESS)
        //        {
        //            if (IsGettingCurrentState)
        //            {
        //                return FixMonoSound;
        //            }
        //            //Change state
        //            else
        //            {
        //                FixMonoSound = !FixMonoSound;
        //
        //                if (const auto Code1 = SetValueFunction(BaseSoftwareKey, L"FixMonoSound", FixMonoSound);
        //                    Code1 == ERROR_SUCCESS)
        //                {
        //                    return FixMonoSound;
        //                }
        //                else
        //                {
        //                    SafePointerDereference(Singleton<VariableMainLogger>::GetInstance().GetTracePtr(), P7_CRITICAL(0, TM("Write key and SetValueFunction(%s, FixMonoSound) error: %s"),
        //                        BaseSoftwarePathGUID.c_str(), ParseWindowsError(Code1).c_str()));
        //
        //                    return !FixMonoSound;
        //                }
        //            }
        //        }
        //        else
        //        {
        //            SafePointerDereference(Singleton<VariableMainLogger>::GetInstance().GetTracePtr(), P7_TRACE(0, TM("GetValueFunction(%s, FixMonoSound) error: %s"),
        //                BaseSoftwarePathGUID.c_str(), ParseWindowsError(Code).c_str()));
        //
        //            //change state
        //            if (!IsGettingCurrentState)
        //            {
        //                FixMonoSound = 1;    //true
        //                if (const auto Code1 = SetValueFunction(BaseSoftwareKey, L"FixMonoSound", FixMonoSound);
        //                    Code1 == ERROR_SUCCESS)
        //                {
        //                    return FixMonoSound;
        //                }
        //                else
        //                {
        //                    SafePointerDereference(Singleton<VariableMainLogger>::GetInstance().GetTracePtr(), P7_CRITICAL(0, TM("Create key and SetValueFunction(%s, FixMonoSound) error: %s"),
        //                        BaseSoftwarePathGUID.c_str(), ParseWindowsError(Code1).c_str()));
        //
        //                    return !FixMonoSound;
        //                }
        //            }
        //
        //            return 0;     //false
        //        }
        //    }
        //    else
        //    {
        //        SafePointerDereference(Singleton<VariableMainLogger>::GetInstance().GetTracePtr(), P7_TRACE(0, TM("TryOpen(%s, GENERIC_READ | KEY_QUERY_VALUE) error: %s"),
        //            BaseSoftwarePathGUID.c_str(), ParseWindowsError(BaseSoftwareKeyValue.Code()).c_str()));
        //
        //        return 0;     //false
        //    }
        //
        //    SafePointerDereference(Singleton<VariableMainLogger>::GetInstance().GetTracePtr(), P7_CRITICAL(0, TM("[REALISATION ERROR] Dead code reached")));
        //    return 0;     //false
        //}

    private:
        AudioDevicePickTable& owner;
        juce::ToggleButton toggleButton;
        int row, columnId;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SelectionFixMonoColumnCustomComponent)
    };

    //friend class SelectionFixMonoColumnCustomComponent;

public:
    void SetApplyChangesButton(juce::TextButton* ApplyChangesButton)
    {
        m_pApplyChangesButton = ApplyChangesButton;
    }

    std::vector<AudioDevice> GetMicrophoneDevies()
    {
        return MicrophoneDevices;
    }

private:
    juce::TableListBox table;

    std::vector<AudioDevice> MicrophoneDevices;

    bool IsApplyChangesButtonActive = false;

    //terrible decision, find something better(maybe eventing).
    //Mutex doesn't seem to be needed(?)
    juce::TextButton* m_pApplyChangesButton = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioDevicePickTable)
};




class OneInstanceAlertWindowComponent : public juce::Component
{
public:
    OneInstanceAlertWindowComponent()
    {
        addAndMakeVisible(WarningLabel);
        WarningLabel.setText("Only one instance of VST can be run\n\Please close this instance of VST", juce::NotificationType::dontSendNotification);
        WarningLabel.setFont(juce::Font(22.f, juce::Font::plain));
        WarningLabel.setColour(juce::Label::textColourId, juce::Colours::orangered);
        WarningLabel.setJustificationType(juce::Justification::centred);
        WarningLabel.setVisible(true);
    }

    void resized() override
    {
        const int Width = getWidth();
        const int Height = getHeight();

        auto area = getLocalBounds();
        area.setCentre(Width / 2, Height / 2 - WarningLabel.getBounds().getHeight() / 2);

        WarningLabel.setBounds(area);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(static_cast<juce::uint8>(242), static_cast<juce::uint8>(243), static_cast<juce::uint8>(244), 0.75f));
    }

private:
    juce::Label WarningLabel;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OneInstanceAlertWindowComponent)
};



//==============================================================================
/**
*/
class MicFusionBridgeAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                             public juce::Button::Listener,
                                             public juce::Timer
{
public:
    MicFusionBridgeAudioProcessorEditor (MicFusionBridgeAudioProcessor&);
    ~MicFusionBridgeAudioProcessorEditor() override;

    void timerCallback() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    void buttonClicked(juce::Button* button) override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    MicFusionBridgeAudioProcessor& audioProcessor;

    const int MarginLeft = 5;
    const int MarginTop = 5;
    const int MarginRight = 5;
    const int MarginBottom = 5;

    const int DividingLine = 3;

    //TODO refactor interface: https://forum.juce.com/t/textbutton-for-process-on-off/23003#:~:text=You%20can%20simply,.
    juce::TextButton SettingsButton;
    juce::TextButton AboutButton;

    class StateComponent : public juce::Component
    {
    public:
        StateComponent(MicFusionBridgeAudioProcessorEditor& ape) : owner(ape), StatePointRadius(0.f)
        {
            this->addAndMakeVisible(StateLabel);

            StateLabel.setText("STATUS: ", juce::NotificationType::dontSendNotification);
            StateLabel.setFont(juce::Font(FontSize, juce::Font::plain));
        }
        ~StateComponent()
        {

        }

    private:
        void paint(juce::Graphics& g) override
        {
            const auto pAPOMainManager = owner.audioProcessor.APOMainManager.get();

            const std::vector<APOAnswerInfo> DeviceInfos = pAPOMainManager ? pAPOMainManager->APOAnswerDevice.load() : std::vector<APOAnswerInfo>();

            const float CurrentSystemTime = GSystemTimer.GetGlobalSystemTime();
            const float LastCallTimeProcessBlock = owner.audioProcessor.fLastCallTimeProcessBlock;
            //Find min call APOProcess time of ALL APOs
            float LastCallTimeAPOProcess = 0.f;
            {
                static float CachedLastCallTimeAPOProcess = 0.f;
                const auto LastCallTimeAPOProcessIterator = std::min_element(DeviceInfos.begin(), DeviceInfos.end(),
                    [](const APOAnswerInfo& Device1, const APOAnswerInfo& Device2)
                    {
                        return Device1.LastTimeAPOProcessCallWithMix < Device2.LastTimeAPOProcessCallWithMix;
                    });
                LastCallTimeAPOProcess = (LastCallTimeAPOProcessIterator != DeviceInfos.end()) ? 
                    LastCallTimeAPOProcessIterator->LastTimeAPOProcessCallWithMix : CachedLastCallTimeAPOProcess;
                CachedLastCallTimeAPOProcess = LastCallTimeAPOProcess;
            }

            constexpr float TimeTarget = 0.3f;    //seconds

            if (std::abs(CurrentSystemTime - LastCallTimeProcessBlock) <= TimeTarget /*seconds*/ &&
                std::abs(CurrentSystemTime - LastCallTimeAPOProcess) > TimeTarget /*seconds*/)
            {
                g.setColour(juce::Colours::red);
            }
            else if (std::abs(CurrentSystemTime - LastCallTimeProcessBlock) > TimeTarget /*seconds*/ &&
                std::abs(CurrentSystemTime - LastCallTimeAPOProcess) > TimeTarget /*seconds*/)
            {
                g.setColour(juce::Colours::orange);
            }
            else
            {
                g.setColour(juce::Colours::green);
            }

            g.fillEllipse(StatePoint.x, StatePoint.y, StatePointRadius, StatePointRadius);
        }

        void resized() override
        {
            auto area = getLocalBounds();

            const int Width = getWidth();
            const int Height = getHeight();
            const int StateLabelEmptySize = StateLabel.getFont().getStringWidth(StateLabel.getText());

            StatePointRadius = FontSize / 1.7;

            area.setLeft(Width / 2 - StateLabelEmptySize / 2 - StatePointRadius / 1.5);
            StateLabel.setBounds(area);
            area.removeFromLeft(StateLabelEmptySize);
            StatePoint.setXY(area.getX(), 1 + /*to make it look better visually*/ Height / 2 - (FontSize * 0.5) / 2);
        }

    private:
        juce::Label StateLabel;
        juce::Point<float> StatePoint;
        float StatePointRadius;

        float FontSize = 24.f;
        
        GlobalSystemTimer<float> GSystemTimer;

        MicFusionBridgeAudioProcessorEditor& owner;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StateComponent)
    };
    StateComponent State{*this};

    juce::Label GainSliderDiscriptionLabel;
    juce::Slider GainSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> GainAttachment1;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> MuteMicrophoneAttachment1;

    //juce::ToggleButton MuteMicrophoneToggleButton;
    juce::TextButton MuteMicrophoneToggleButton;

    juce::Label PluginNameLabel;

    AudioDevicePickTable AudioTable;

    juce::TextButton ApplyChangesButton;

    juce::TextButton RestartAudioServiceButton;

    OneInstanceAlertWindowComponent InstanceAlertComponent;

    //Log puprose only
    std::atomic<int64_t> PluginEditorInstanceCount = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MicFusionBridgeAudioProcessorEditor)
};
