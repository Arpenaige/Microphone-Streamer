#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
MicFusionBridgeAudioProcessor::MicFusionBridgeAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     :  /*crashdumper(),*/
        AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
    treeState(*this, nullptr, "PARAMETERS", CreateParameters())
#endif
{
    SafePointerDereference(Singleton<VariableMainLogger>::GetInstance().GetTracePtr(), P7_TRACE(0, TM("MicFusionBridgeAudioProcessor()")));

    SetLastError(ERROR_SUCCESS);
    OneInstanceAppMutex.reset(CreateMutexW(nullptr, true, OneInstanceAppMutexString));
    if (OneInstanceAppMutex.is_valid())
    {
        if (GetLastError() == ERROR_ALREADY_EXISTS)
        {
            OneInstanceAppMutex.reset();
            IsOneInstanceLaunched = false;
        }
        else
        {
            IsOneInstanceLaunched = true;
        }
    }
    else
    {
        IsOneInstanceLaunched = false;

        SafePointerDereference(Singleton<VariableMainLogger>::GetInstance().GetTracePtr(), P7_CRITICAL(0, TM("CreateMutexW for OneInstanceAppMutexString  = [%s] error: %s"),
            OneInstanceAppMutexString, ParseWindowsError(GetLastError()).c_str()));
    }

    if (IsOneInstanceLaunched)
    {
        APOMainManager = std::make_unique<APOManager>();
    }
    //else
    //{
    //    APOMainManager.reset();
    //}
}

MicFusionBridgeAudioProcessor::~MicFusionBridgeAudioProcessor()
{
    SafePointerDereference(Singleton<VariableMainLogger>::GetInstance().GetTracePtr(), P7_TRACE(0, TM("~MicFusionBridgeAudioProcessor()")));
}

//==============================================================================
const juce::String MicFusionBridgeAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool MicFusionBridgeAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool MicFusionBridgeAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool MicFusionBridgeAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double MicFusionBridgeAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int MicFusionBridgeAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int MicFusionBridgeAudioProcessor::getCurrentProgram()
{
    return 0;
}

void MicFusionBridgeAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String MicFusionBridgeAudioProcessor::getProgramName (int index)
{
    return {};
}

void MicFusionBridgeAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void MicFusionBridgeAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    SafePointerDereference(Singleton<VariableMainLogger>::GetInstance().GetTracePtr(), P7_TRACE(0, TM("prepareToPlay(sampleRate = %lf, samplesPerBlock = %d)"),
        sampleRate, samplesPerBlock));

    pDbValue = treeState.getRawParameterValue(GAIN_ID1);
    pIsMutedMicrophone = treeState.getRawParameterValue(MuteMicrophone_ID1);

    if (SamplesBufferDynamicSize < samplesPerBlock * 2)
    {
        SamplesBufferDynamicSize = samplesPerBlock * 2;
        SamplesBufferDynamic = std::make_unique_for_overwrite<float[]>(SamplesBufferDynamicSize);
    }

    //if (true /*AVX is support, check cpuid*/)
    //{
    //    InterleaveSamplesInstanse = std::make_unique<InterleaveSamplesAVX>();
    //}
}

void MicFusionBridgeAudioProcessor::releaseResources()
{
    SafePointerDereference(Singleton<VariableMainLogger>::GetInstance().GetTracePtr(), P7_TRACE(0, TM("releaseResources()")));

    SamplesBufferDynamicSize = 0ull;
    SamplesBufferDynamic.reset();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool MicFusionBridgeAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void MicFusionBridgeAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    //auto totalNumInputChannels  = getTotalNumInputChannels();
    //auto totalNumOutputChannels = getTotalNumOutputChannels();

    if (!APOMainManager.get())
    {
        return;
    }

    fLastCallTimeProcessBlock = GSystemTimer.GetGlobalSystemTime();

    const float db = *pDbValue;
    const float dbGain = std::pow(10.f, (db / 20.f));


    const auto NumChannels = buffer.getNumChannels();
    const auto Numsamples = buffer.getNumSamples();

    if (SamplesBufferDynamicSize < Numsamples * NumChannels)
    {
        SamplesBufferDynamicSize = Numsamples * NumChannels;
        SamplesBufferDynamic = std::make_unique_for_overwrite<float[]>(SamplesBufferDynamicSize);
    }




    for (int i = 0; i < NumChannels; ++i)
    {
        const float* ReadPointer = buffer.getReadPointer(i);
        for (int j = 0; j < Numsamples; ++j)
        {
            SamplesBufferDynamic[i + j * NumChannels] = ReadPointer[j] * dbGain;
        }
    }

    const bool IsMutedMicrophone = *pIsMutedMicrophone;

    uint16_t BitMask = 0ul;
    if (IsMutedMicrophone)
    {
        BitMask |= BitMaskSendSamples::APOMuteMainSignal;
    }
    APOMainManager->SendSamplesAsync(SamplesBufferDynamic.get(), NumChannels * Numsamples, getSampleRate(), NumChannels, BitMask);



    ////Writing an output buffer from buffer for APO
    //for (size_t i = 0; i < buffer.getNumChannels(); i++)
    //{
    //    for (size_t j = 0; j < buffer.getNumSamples(); j++)
    //    {
    //        *buffer.getWritePointer(i, j) = SamplesBufferDynamic[i + j * buffer.getNumChannels()];
    //    }
    //}
}

//==============================================================================
bool MicFusionBridgeAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* MicFusionBridgeAudioProcessor::createEditor()
{
    return new MicFusionBridgeAudioProcessorEditor (*this);
}

//==============================================================================
void MicFusionBridgeAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    SafePointerDereference(Singleton<VariableMainLogger>::GetInstance().GetTracePtr(), P7_TRACE(0, TM("getStateInformation(destData.getSize() = %llu)"),
        destData.getSize()));

    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.

    auto state = treeState.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void MicFusionBridgeAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.

    SafePointerDereference(Singleton<VariableMainLogger>::GetInstance().GetTracePtr(), P7_TRACE(0, TM("setStateInformation(sizeInBytes = %d)"),
        sizeInBytes));

    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState.get())
    {
        if (xmlState->hasTagName(treeState.state.getType()))
        {
            treeState.replaceState(juce::ValueTree::fromXml(*xmlState));
        }
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MicFusionBridgeAudioProcessor();
}


juce::AudioProcessorValueTreeState::ParameterLayout MicFusionBridgeAudioProcessor::CreateParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;
    int versionNum = 1;
    using PBool = juce::AudioParameterBool;
    //using PInt = juce::AudioParameterInt;
    using PFloat = juce::AudioParameterFloat;
    parameters.push_back(std::make_unique<PFloat>(juce::ParameterID{ GAIN_ID1, versionNum }, GAIN_NAME1, juce::NormalisableRange<float>(-48., 15., 0.1), /*-16.0f*/0.f));
    parameters.push_back(std::make_unique<PBool>(juce::ParameterID{ MuteMicrophone_ID1, versionNum }, MuteMicrophone_NAME1, false));

    return { parameters.begin(), parameters.end() };
}