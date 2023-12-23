#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
MicFusionBridgeAudioProcessorEditor::MicFusionBridgeAudioProcessorEditor (MicFusionBridgeAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    SafePointerDereference(Singleton<VariableMainLogger>::GetInstance().GetTracePtr(), P7_TRACE(0, TM("MicFusionBridgeAudioProcessorEditor(PluginEditorInstanceCount = %lld)"),
        PluginEditorInstanceCount.operator++()));

    juce::Timer::startTimerHz(10.0f);

    this->addAndMakeVisible(SettingsButton);
    this->addAndMakeVisible(AboutButton);
    this->addAndMakeVisible(State);
    this->addAndMakeVisible(GainSliderDiscriptionLabel);
    this->addAndMakeVisible(GainSlider);
    this->addAndMakeVisible(PluginNameLabel);
    //MuteMicrophoneToggleButton.setClickingTogglesState(true);
    this->addAndMakeVisible(MuteMicrophoneToggleButton);

    this->addAndMakeVisible(AudioTable);
    AudioTable.setVisible(false);
    this->addAndMakeVisible(ApplyChangesButton);
    ApplyChangesButton.setVisible(false);
    this->addAndMakeVisible(RestartAudioServiceButton);
    RestartAudioServiceButton.setVisible(false);

    if (!audioProcessor.IsOneInstanceLaunched)
    {
        this->addAndMakeVisible(InstanceAlertComponent);
    }

    SettingsButton.setButtonText("Settings");
    SettingsButton.setToggleable(true);
    SettingsButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::dimgrey);
    AboutButton.setButtonText("About");
    AboutButton.setToggleable(true);
    AboutButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::dimgrey);
    GainSliderDiscriptionLabel.setText("Microphone mix input level", juce::NotificationType::dontSendNotification);
    GainSliderDiscriptionLabel.setFont(juce::Font(22.f, juce::Font::plain));
    GainSlider.setHelpText("Test");
    GainSlider.setTextValueSuffix(" db");
    PluginNameLabel.setText("Streaming\nMicrophone plugin", juce::NotificationType::dontSendNotification);
    PluginNameLabel.setFont(juce::Font(30.f, juce::Font::plain));
    PluginNameLabel.setJustificationType(juce::Justification::centred);
    PluginNameLabel.setColour(juce::Label::textColourId, juce::Colours::orange);
    MuteMicrophoneToggleButton.setToggleable(true);    //ToggleState == 0
    MuteMicrophoneToggleButton.setClickingTogglesState(true);
    MuteMicrophoneToggleButton.setButtonText("Microphone UNMUTED");
    MuteMicrophoneToggleButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::orangered);
    MuteMicrophoneToggleButton.setColour(juce::TextButton::buttonColourId, juce::Colours::yellowgreen);
    //MuteMicrophoneToggleButton.setColour(juce::TextButton::textColourOffId, juce::Colours::gainsboro);
    //MuteMicrophoneToggleButton.setColour(juce::TextButton::textColourOnId, juce::Colours::gainsboro);

    ApplyChangesButton.setButtonText("Apply");
    ApplyChangesButton.setEnabled(false);
    RestartAudioServiceButton.setButtonText("Restart Audio Service");

    SettingsButton.addListener(this);
    AboutButton.addListener(this);
    ApplyChangesButton.addListener(this);
    RestartAudioServiceButton.addListener(this);
    //FixMonoMicrophone.addListener(this);
    MuteMicrophoneToggleButton.addListener(this);

    AudioTable.SetApplyChangesButton(&ApplyChangesButton);





    using SliderAttach = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttach = juce::AudioProcessorValueTreeState::ButtonAttachment;
    GainAttachment1 = std::make_unique<SliderAttach>(audioProcessor.treeState, GAIN_ID1, GainSlider);
    MuteMicrophoneAttachment1 = std::make_unique<ButtonAttach>(audioProcessor.treeState, MuteMicrophone_ID1, MuteMicrophoneToggleButton);
    //MuteMicrophoneToggleButton.getToggleStateValue().referTo(audioProcessor.treeState.state.getPropertyAsValue(MuteMicrophone_ID1, nullptr));

    this->setResizable(false, false);  //Use static size window

    this->setSize (400, 500);
}

MicFusionBridgeAudioProcessorEditor::~MicFusionBridgeAudioProcessorEditor()
{
    SafePointerDereference(Singleton<VariableMainLogger>::GetInstance().GetTracePtr(), P7_TRACE(0, TM("MicFusionBridgeAudioProcessorEditor(PluginEditorInstanceCount = %lld)"),
        PluginEditorInstanceCount.operator--()));

    SettingsButton.removeListener(this);
    AboutButton.removeListener(this);
    ApplyChangesButton.removeListener(this);
    RestartAudioServiceButton.removeListener(this);
    MuteMicrophoneToggleButton.removeListener(this);
}

void MicFusionBridgeAudioProcessorEditor::timerCallback()
{
    repaint();
}

//==============================================================================

/*
* std::ifstream fin("\\JUCECOLOR.txt");
* std::string ColorStr; std::getline(fin, ColorStr);
* g.fillAll(juce::Colours::findColourForName(ColorStr, juce::Colours::transparentBlack));
*/
void MicFusionBridgeAudioProcessorEditor::paint (juce::Graphics& g)
{
    const int Width = getWidth();
    const int Height = getHeight();

    // (Our component is opaque, so we must completely fill the background with a solid colour)
    //g.fillAll(getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
    g.fillAll(juce::Colours::darkslategrey);

    g.setColour(juce::Colours::lavender);
    g.fillRect(0, MarginTop + Height * 0.06 + (DividingLine - 1), Width, (DividingLine - 1));
    if (!(SettingsButton.getToggleState() || AboutButton.getToggleState()))
    {
        g.fillRect(0, Height - (MarginTop + Height * 0.15 - (DividingLine - 1)), Width, (DividingLine - 1));
    }
}


void MicFusionBridgeAudioProcessorEditor::resized()
{
    const int Width = getWidth();
    const int Height = getHeight();

    SafePointerDereference(Singleton<VariableMainLogger>::GetInstance().GetTracePtr(), P7_TRACE(0, TM("MicFusionBridgeAudioProcessorEditor::resized(Width = %d, Height = %d)"),
        Width, Height));

    auto area = getLocalBounds();

    InstanceAlertComponent.setBounds(area);

    area.removeFromLeft(MarginLeft);
    area.removeFromRight(MarginRight);
    area.removeFromTop(MarginTop);
    SettingsButton.setBounds(area.removeFromLeft(Width * 0.3f).removeFromTop(Height * 0.06f));
    AboutButton.setBounds(area.removeFromRight(Width * 0.3f).removeFromTop(Height * 0.06f));

    area.removeFromTop(AboutButton.getBounds().getHeight());
    ////int StateLabelEmptySize = StateLabel.getFont().getStringWidth(StateLabel.getText());
    ////area.setLeft(MarginLeft);
    ////area.removeFromLeft(Width / 2 - StateLabelEmptySize + StatePointRadius);
    ////StatePoint.setXY(area.getCentre().getX(), area.getTopRight().getY());
    ////StateLabel.setBounds(area.removeFromTop(AboutButton.getBounds().getHeight()));
    //State.setBounds(0, 100, Width, Height * 0.1);




    //State.setBounds(0, 100, Width, Height * 0.1);
    area.removeFromTop(DividingLine);
    juce::Rectangle BaseArea = area; BaseArea.removeFromTop(1); BaseArea.setLeft(MarginLeft); BaseArea.setRight(Width - MarginRight);
    State.setBounds(area.removeFromTop(Height * 0.1f));


    area.setRight(Width);
    area.setLeft(0);
    area.removeFromLeft(Width / 2 - GainSliderDiscriptionLabel.getFont().getStringWidth(GainSliderDiscriptionLabel.getText()) / 2);
    GainSliderDiscriptionLabel.setBounds(area.removeFromTop(Height * 0.04f));


    area.setRight(Width - MarginRight);
    area.setLeft(MarginLeft);
    GainSlider.setBounds(area.removeFromTop(Height * 0.08f));

    area.removeFromLeft(Width * 0.1f);
    area.removeFromRight(Width * 0.1f);
    MuteMicrophoneToggleButton.setBounds(area.removeFromTop(Height * 0.06f));



    area.setLeft(MarginLeft);
    area.setRight(Width - MarginRight);
    //area.setBottom(0);
    //area.removeFromTop(0.12 * Height);
    PluginNameLabel.setBounds(area.removeFromBottom(Height * 0.15f));




    AudioTable.setBounds(BaseArea.removeFromTop(Height * 0.7f));

    BaseArea.removeFromLeft(Width * 0.15f);
    BaseArea.removeFromRight(Width * 0.15f);
    ApplyChangesButton.setBounds(BaseArea.removeFromTop(Height * 0.075f));

    BaseArea.removeFromTop(5);

    BaseArea.setLeft(MarginLeft);
    BaseArea.setRight(Width - MarginRight);
    BaseArea.removeFromLeft(Width * 0.05f);
    BaseArea.removeFromRight(Width * 0.05f);
    RestartAudioServiceButton.setBounds(BaseArea.removeFromTop(Height * 0.075f));
}


void MicFusionBridgeAudioProcessorEditor::buttonClicked(juce::Button* button)
{
    if (button == &SettingsButton)
    {
        SettingsButton.setToggleState(!SettingsButton.getToggleState(), juce::NotificationType::dontSendNotification);
        AboutButton.setToggleState(false, juce::NotificationType::dontSendNotification);

        if (SettingsButton.getToggleState())
        {
            State.setVisible(false);
            GainSlider.setVisible(false);
            GainSliderDiscriptionLabel.setVisible(false);
            PluginNameLabel.setVisible(false);
            MuteMicrophoneToggleButton.setVisible(false);

            AudioTable.update();
            AudioTable.repaint();
            AudioTable.resized();
            AudioTable.setVisible(true);
            ApplyChangesButton.setVisible(true);
            RestartAudioServiceButton.setVisible(true);
        }
        else
        {
            State.setVisible(true);
            GainSlider.setVisible(true);
            GainSliderDiscriptionLabel.setVisible(true);
            PluginNameLabel.setVisible(true);
            MuteMicrophoneToggleButton.setVisible(true);

            AudioTable.setVisible(false);
            ApplyChangesButton.setVisible(false);
            RestartAudioServiceButton.setVisible(false);
        }

        repaint();
    }
    if (button == &AboutButton)
    {
        AboutButton.setToggleState(!AboutButton.getToggleState(), juce::NotificationType::dontSendNotification);
        SettingsButton.setToggleState(false, juce::NotificationType::dontSendNotification);

        if (AboutButton.getToggleState())
        {
            State.setVisible(false);
            GainSlider.setVisible(false);
            GainSliderDiscriptionLabel.setVisible(false);
            PluginNameLabel.setVisible(false);
            MuteMicrophoneToggleButton.setVisible(false);

            AudioTable.setVisible(false);
            ApplyChangesButton.setVisible(false);
            RestartAudioServiceButton.setVisible(false);
        }
        else
        {
            State.setVisible(true);
            GainSlider.setVisible(true);
            GainSliderDiscriptionLabel.setVisible(true);
            PluginNameLabel.setVisible(true);
            MuteMicrophoneToggleButton.setVisible(true);

            AudioTable.setVisible(false);
            ApplyChangesButton.setVisible(false);
            RestartAudioServiceButton.setVisible(false);
        }

        repaint();
    }
    if (button == &ApplyChangesButton)
    {
        std::vector<AudioDevice> MicrophoneDevices = AudioTable.GetMicrophoneDevies();

        std::wstring res;
        for (const AudioDevice& Device: MicrophoneDevices)
        {
            if (Device.GetNeedUpdate())
            {
                res += std::format(L"{}{} {}", res.empty() ? L"" : L" ", Device.GetDeviceGUID(), Device.GetNeedInstalling() ? L"i" : L"u");
            }
            //TODO: to make sure that only the modified device is recorded.
        }

        ManageAPO(ManageAPOParams::Install_DeInstall_AudioDevices, res.c_str());

        AudioTable.update();

        repaint();
    }
    if (button == &RestartAudioServiceButton)
    {
        const juce::MessageBoxOptions Options = juce::MessageBoxOptions()
            .withIconType(juce::MessageBoxIconType::WarningIcon)
            .withTitle("Do you really want to restart the Windows Audio service ?")
            .withMessage("Restarting the Audio Windows service may result in not all applications or DAWs being able to reconnect to the audio device(s).\n"
               "You may need to restart your current DAW or some application on your system"
               "For example - Discord(Try recconect to channel, if didn't help - restart)")
            .withButton("Yes")
            .withButton("No");


        juce::AlertWindow::showAsync(Options, [](int ButtonIndex)
            {
                if (ButtonIndex == 1)
                {
                    const int RestartCode = ManageAPO(ManageAPOParams::RestartWindowsAudio);

                    const juce::MessageBoxOptions Options = juce::MessageBoxOptions()
                        .withIconType(juce::MessageBoxIconType::InfoIcon)
                        .withTitle("Status of Restart Windows Audio Service")
                        .withMessage(RestartCode == 0 ? "Restart success" : std::string("Restart failed with code: ") + std::to_string(RestartCode))
                        .withButton("OK");

                    juce::AlertWindow::showAsync(Options, [](int)
                        {
                            //TODO: after this, update status of APO
                            //TODO: Also add a label with status (like restart required).
                        });
                }
            });

        AudioTable.update();


        repaint();
    }
    if (button == &MuteMicrophoneToggleButton)
    {
        if (MuteMicrophoneToggleButton.getToggleState())
        {
            MuteMicrophoneToggleButton.setButtonText("Microphone MUTED");
        }
        else
        {
            MuteMicrophoneToggleButton.setButtonText("Microphone UNMUTED");
        }

        //std::cout << "MuteMicrophoneToggleButton ToggleState current: " << MuteMicrophoneToggleButton.getToggleState() << '\n';

        repaint();
    }
}


//#define UseExtrenalConsole 1
#ifdef UseExtrenalConsole
#include <Windows.h>
void ConsoleLoad()
{
    AllocConsole();
    freopen_s((FILE**)stdin, "CONIN$", "r", stdin);
    freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
    freopen_s((FILE**)stderr, "CONOUT$", "w", stderr);
    SetConsoleTitleA("Microphone Streamer VST Debug Console");

    setlocale(LC_ALL, "ru");

    //const HWND hwndMyWnd = FindWindow("ConsoleWindowClass", "C:\\WINDOWS\\system32\\cmd.exe");
    const HWND hwndMyWnd = GetConsoleWindow();

    ::SetWindowPos(hwndMyWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_DRAWFRAME | SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    ::ShowWindow(hwndMyWnd, SW_NORMAL);

    HANDLE handleOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD consoleMode;
    GetConsoleMode(handleOut, &consoleMode);
    consoleMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    consoleMode |= DISABLE_NEWLINE_AUTO_RETURN;
    SetConsoleMode(handleOut, consoleMode);
}
void ConsoleUnload()
{
    fclose((FILE*)stdin);
    fclose((FILE*)stdout);
    fclose((FILE*)stderr);
    FreeConsole();
}


class ConsoleRAII
{
public:
    ConsoleRAII()
    {
        ConsoleLoad();
    }
    ~ConsoleRAII()
    {
        std::cout << "Console closed\n";

        ConsoleUnload();
    }
};
ConsoleRAII raii;
#endif
