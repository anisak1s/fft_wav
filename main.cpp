#include <iostream>
#include <utility>
#include <vector>
#include "libwav.hpp"
#include "libplot.hpp"
#include <fftw3.h>
#include <cmath>
#include <SDL.h>

#define assert_error {cout << "Error: " << __FILE__ << ":" << __LINE__ << endl;return -1;}

using std::cout;
using std::endl;
using std::min;

constexpr int fft_n = 4410*2;
constexpr int fft_mul = 1;
constexpr int audio_buf = 1024;
int main(int args,char** argc) {
    int files = 1;
    if(args < 3) {
        cout << "Invalid argument" << endl;
        assert_error
    }
    PlotWindow plot(1600,900);
    //PlotWindow plot(1920,1080);
    plot.SetGrid(40,14000,0,0,10,10,PlotWindow::GridMode::Logarithm,PlotWindow::GridMode::Logarithm);
    if(!plot.isReady()) assert_error
    std::vector<std::pair<double,double>> dat,dat2;
    MSWavFile wav(argc[1]);
    cout << "SampleRate: " << wav.getSampleRate() << " Hz" << endl;
    cout << "Channels: " << wav.getChannels() << endl;
    cout << "Samples: " << wav.getSampleCount() << endl;
    cout << "Resolution: " << wav.getBitsPerSample() << endl;
    if(SDL_Init(SDL_INIT_AUDIO) != 0) assert_error
    if(SDL_AudioInit(SDL_GetAudioDriver(0)) != 0) assert_error
    SDL_AudioSpec want,have;
    want.freq = wav.getSampleRate();
    want.format = AUDIO_S16;
    want.channels = wav.getChannels();
    want.samples = audio_buf;
    want.callback = NULL;
    SDL_AudioDeviceID aid = SDL_OpenAudioDevice(NULL,0,&want,&have,SDL_AUDIO_ALLOW_ANY_CHANGE);
    if(aid == 0) assert_error
    SDL_PauseAudioDevice(aid,0);
    long frames = 0;
    long count = 0;
    fftw_complex *in,*out;
    fftw_plan p,r;

    in = (fftw_complex*)fftw_malloc(sizeof(fftw_complex)*fft_n*fft_mul);
    out = (fftw_complex*)fftw_malloc(sizeof(fftw_complex)*fft_n*fft_mul);
    p = fftw_plan_dft_1d(fft_n*fft_mul,in,out,FFTW_FORWARD,FFTW_ESTIMATE);

    while(count + fft_n < wav.getSampleCount()) {
        int r = plot.HandleEvent();
        if(r == 1) break;

        for(int i = 0;i < fft_n;i++) {
            auto d = (int*)((char *) wav.getRawBuffer() + ((count+i) * wav.getChannels() * (wav.getBitsPerSample() / 8)));
	    for(int j = 0;j < fft_mul;j++) {
            	in[i*fft_mul+j][0] = (double)(*d) / (double)INT16_MAX;
            	in[i*fft_mul+j][1] = 0;
	    }
        }
        fftw_execute(p);
        dat.clear();
        for(int i = 1;i < (fft_n*fft_mul)/2;i++) {
            dat.push_back(std::make_pair((double)i * ((double)wav.getSampleRate()) / ((double)fft_n*fft_mul),abs(out[i][0])));
        }
    char buf[1024];
	if(r == 2) {
            sprintf(buf,"%s.%d",argc[2],files);
            files++;
            FILE* f = fopen(buf,"w");
            for(auto e:dat) {
                fprintf(f,"%024.12f %024.12f\n",e.first,e.second);
	    }
	    fclose(f);
	    cout << "Transformed data dumped." << endl;
	}
        if(dat2.size() < dat.size()) {
            dat2.clear();
            for(auto& e:dat) dat2.push_back(e);
        }
        for(int i = 0;i < dat2.size();i++) {
            //dat2[i].second = std::max(dat[i].second,dat2[i].second * 0.8);
            dat2[i].second = dat[i].second*0.3+dat2[i].second * 0.7;
        }
        plot.DrawLineGraph(dat2);
        
        long b2wrt = min((long)(audio_buf*2-SDL_GetQueuedAudioSize(aid)),(long)((wav.getSampleCount()-count)*wav.getChannels()*(wav.getBitsPerSample()/8)));
        frames = SDL_QueueAudio(aid,(char *) wav.getRawBuffer() + (count * wav.getChannels() * (wav.getBitsPerSample() / 8)),b2wrt);
        if (frames != 0) assert_error
        count += b2wrt / (wav.getChannels() * (wav.getBitsPerSample()/8));

    }
    SDL_CloseAudioDevice(aid);
    return 0;

    while(1) {
        int r = plot.HandleEvent();
	if(r == 0) break;
    }
    return 0;
}
