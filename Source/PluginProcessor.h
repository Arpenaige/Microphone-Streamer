#pragma once

#include <JuceHeader.h>

#include <format>
#include <iostream>
#include <string_view>

#include <Common.h>
#include <APOManager.h>
#include <GlobalSystemTimer.hpp>
#include <Message.h>

#include <Windows.h>
#include <dbghelp.h>
#include <shellapi.h>
#include <shlobj.h>

//TODO: add to JUCE configurator
#pragma comment(lib, "Dbghelp.lib")

#include <Singleton.hpp>



//TODO: stable this code
//class CrashDumper
//{
//public:
//    CrashDumper()
//    {
//        SafePointerDereference(Singleton<VariableMainLogger>::GetInstance().GetTracePtr(), P7_TRACE(0, TM("CrashDumper()"))); P7_Flush();
//
//        //WARN: all __try __except being catched and writed minidump
//        hVectorException = AddVectoredExceptionHandler(1, VectoredHandler);
//        if (!hVectorException)
//        {
//            SafePointerDereference(Singleton<VariableMainLogger>::GetInstance().GetTracePtr(), P7_CRITICAL(0, TM("AddVectoredExceptionHandler GetLastError(): %d"),
//                GetLastError())); P7_Flush();
//            //Error
//        }
//    }
//    ~CrashDumper()
//    {
//        SafePointerDereference(Singleton<VariableMainLogger>::GetInstance().GetTracePtr(), P7_TRACE(0, TM("~CrashDumper()"))); P7_Flush();
//
//        if (hVectorException)
//        {
//            if (ULONG RemoveExceptionHanlderValue = RemoveVectoredExceptionHandler(hVectorException);
//                !RemoveExceptionHanlderValue)
//            {
//                SafePointerDereference(Singleton<VariableMainLogger>::GetInstance().GetTracePtr(), P7_CRITICAL(0, TM("RemoveVectoredExceptionHandler GetLastError(): %d"),
//                    GetLastError())); P7_Flush();
//            }
//        }
//    }
//
//private:
//    static LONG WINAPI VectoredHandler(_EXCEPTION_POINTERS* ExceptionInfo)
//    {
//        SafePointerDereference(Singleton<VariableMainLogger>::GetInstance().GetTracePtr(), P7_INFO(0, TM("VectoredHandler called"))); P7_Flush();
//
//        GenerateDump(ExceptionInfo);
//
//        return EXCEPTION_CONTINUE_SEARCH;
//    }
//
//    //https://learn.microsoft.com/ru-ru/windows/win32/dxtecharts/crash-dump-analysis
//    static int GenerateDump(EXCEPTION_POINTERS* pExceptionPointers)
//    {
//        const wchar_t* szAppName = L"MicrophoneStreamer";
//        const wchar_t* szVersion = L"v1.0.1";
//
//        const DWORD dwBufferSize = MAX_PATH;
//        WCHAR szPath[dwBufferSize];
//        const DWORD size = GetTempPathW(dwBufferSize, szPath);
//        if (!size)
//        {
//            SafePointerDereference(Singleton<VariableMainLogger>::GetInstance().GetTracePtr(), P7_CRITICAL(0, TM("GetTempPathW GetLastError(): %d"),
//                GetLastError())); P7_Flush();
//            return EXCEPTION_EXECUTE_HANDLER;
//        }
//
//        std::wstring szFileName = std::format(L"{}{}", szPath, L"ControlAPO");
//
//        SYSTEMTIME stLocalTime;
//        GetLocalTime(&stLocalTime);
//
//        szFileName += std::format(L"\\{}-{:04d}{:02d}{:02d}-{:02d}{:02d}{:02d}-{}-{}.dmp",
//            szVersion,
//            stLocalTime.wYear, stLocalTime.wMonth, stLocalTime.wDay,
//            stLocalTime.wHour, stLocalTime.wMinute, stLocalTime.wSecond,
//            GetCurrentProcessId(), GetCurrentThreadId());
//
//        std::wcout << L"path: " << szFileName << L'\n';
//
//        wil::unique_handle hDumpFile;
//        hDumpFile.reset(CreateFileW(szFileName.c_str(), GENERIC_READ | GENERIC_WRITE,
//            FILE_SHARE_WRITE | FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0));
//        if (!hDumpFile)
//        {
//            SafePointerDereference(Singleton<VariableMainLogger>::GetInstance().GetTracePtr(), P7_CRITICAL(0, TM("CreateFileW GetLastError(): %d"),
//                GetLastError())); P7_Flush();
//            return EXCEPTION_EXECUTE_HANDLER;
//        }
//
//        MINIDUMP_EXCEPTION_INFORMATION ExpParam;
//        ExpParam.ThreadId = GetCurrentThreadId();
//        ExpParam.ExceptionPointers = pExceptionPointers;
//        ExpParam.ClientPointers = TRUE;
//
//        SafePointerDereference(Singleton<VariableMainLogger>::GetInstance().GetTracePtr(), P7_TRACE(0, TM("Crush Dump Current Thread ID = %lu"),
//            GetCurrentThreadId())); P7_Flush();
//
//        BOOL bMiniDumpSuccessful = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),
//            hDumpFile.get(), MiniDumpWithDataSegs, &ExpParam, NULL, NULL);
//        if (!bMiniDumpSuccessful)
//        {
//            SafePointerDereference(Singleton<VariableMainLogger>::GetInstance().GetTracePtr(), P7_CRITICAL(0, TM("MiniDumpWriteDump GetLastError(): %d"),
//                GetLastError())); P7_Flush();
//            return EXCEPTION_EXECUTE_HANDLER;
//        }
//
//        return EXCEPTION_EXECUTE_HANDLER;
//    }
//
//private:
//    PVOID hVectorException;
//};
//CrashDumper CrashDumperRAII;
//Singleton<CrashDumper>;

//==============================================================================
/**
*/
class MicFusionBridgeAudioProcessor  : public juce::AudioProcessor
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorARAExtension
                            #endif
{
public:
    //==============================================================================
    MicFusionBridgeAudioProcessor();
    ~MicFusionBridgeAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

public:
    //Based on https://github.com/jerryuhoo/Fire
    juce::AudioProcessorValueTreeState treeState;
    juce::AudioProcessorValueTreeState::ParameterLayout CreateParameters();

    std::atomic<float>* pDbValue;
    std::atomic<float>* pIsMutedMicrophone;


    std::unique_ptr<float[]> SamplesBufferDynamic;
    size_t SamplesBufferDynamicSize = 0;


    std::atomic<float> fLastCallTimeProcessBlock = 0.f;


    std::unique_ptr<APOManager> APOMainManager;


    GlobalSystemTimer<float> GSystemTimer;

    //CrashDumper crashdumper;

    //std::unique_ptr<InterleaveSamples> InterleaveSamplesInstanse;

    wil::unique_handle OneInstanceAppMutex;
    bool IsOneInstanceLaunched;

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MicFusionBridgeAudioProcessor)
};
