#include <stdio.h>
#include <iostream>
#include <chrono>
#include <vector>
#include <SDL2/SDL.h>

#include <assert.h>

const char* riff_magic = "RIFF";



/* Code pulled from https://github.com/bisqwit/speech_synth_series/blob/master/ep2-pcmaudio/pcmaudio-tiny2.cc*/
struct AudioPlayer
{
    SDL_AudioSpec spec {};
    short* file;
    bool isFileDone = false;

    AudioPlayer()
    {
        spec.freq     = 48000;         // boilerplate
        spec.format   = AUDIO_S16SYS;  
        spec.channels = 2;             
        spec.samples  = 4096;          
        spec.userdata = this;
        spec.callback = [](void* param, Uint8* stream, int len)
                        {
                            ((AudioPlayer*)param)->callback((short*) stream, len / sizeof(short));
                        };
        auto dev = SDL_OpenAudioDevice(nullptr, 0, &spec, &spec, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
        SDL_PauseAudioDevice(dev, 0);
    }


    unsigned int offset = 0;
    unsigned int sampleCount=0;
    void callback(short* target, int num_samples)
    {
        //printf("Copying Offset#%i...",offset/num_samples);
        memset(target,0,sizeof(short)*num_samples);
        memcpy(target,file+offset,num_samples*sizeof(short));

        offset+= num_samples;

        if (offset > sampleCount)
        {
            isFileDone = true;
        }
        

    }
};




int main(int argc, char const *argv[])
{
    

    if (argc < 2)
    {
        printf("Syntax: ./wavplay [filename.wav]\n");
        return EXIT_FAILURE;
    }
    

    FILE* fp = fopen(argv[1],"rb");
    if (!fp)
    {
        printf("%s not found!\n",argv[1]);
        return EXIT_FAILURE;
    }

    
    char check[4];

    fread(check,1,4,fp);

    assert(memcmp(check,riff_magic,4) == 0);
    

    unsigned int file_size;
    fread(&file_size,sizeof(unsigned int),1,fp);

    //printf("File Size - 8: %i\n",file_size);

    fseek(fp,8,SEEK_CUR); /* Skip "WAVE" and "fmt "*/
    
    /* fmt chunk */
    /*  fmt_size_per_sec = (Sample Rate * BitsPerSample * Channels) / 8 
        fmt_what = (BitsPerSample * Channels) / 8.1 - 8 bit mono2 - 8 bit stereo/16 bit mono4 - 16 bit stereo
    */

    unsigned int fmt_size,fmt_sample_rate,fmt_size_per_sec;
    short fmt_type, fmt_channels, fmt_what, fmt_bps;
    unsigned int fmt_data_size;
    


    fread(&fmt_size,sizeof(unsigned int),1,fp);
    fread(&fmt_type,sizeof(short),1,fp);
    fread(&fmt_channels,sizeof(short),1,fp);
    fread(&fmt_sample_rate,sizeof(unsigned int),1,fp);
    fread(&fmt_size_per_sec,sizeof(unsigned int),1,fp);
    fread(&fmt_what,sizeof(short),1,fp);
    fread(&fmt_bps,sizeof(short),1,fp);
    


    printf("RIFF/WAVE File: %s format, %i bit, %s, %i Hz\n",(fmt_type==1) ? "PCM" : "?",fmt_bps,(fmt_channels==2) ? "stereo" : (fmt_channels == 1) ? "mono" : "?",fmt_sample_rate);

    const char* data_hdr = "data";
    char chk[4] = {0,0,0,0};
    while (memcmp(data_hdr,chk,4) != 0)
    {
        fseek(fp,-3,SEEK_CUR);
        fread(chk,1,4,fp);
    }
    printf("Found data @ offset: %i\n",(int)ftell(fp));

    fread(&fmt_data_size,sizeof(unsigned int),1,fp);

    unsigned int sampleCnt = (file_size - ftell(fp)) / ((fmt_bps / 8));



    short* samples = new short[sampleCnt];
    memset(samples,0,sampleCnt);
    fread(samples,fmt_bps/8,sampleCnt,fp);
    

    /* Actually play sound */
    SDL_InitSubSystem(SDL_INIT_AUDIO);
    AudioPlayer engine;
    engine.file = samples;
    engine.spec.freq = fmt_sample_rate;
    engine.spec.channels = fmt_channels;
    engine.sampleCount = sampleCnt;

    

    int audioTime = (sampleCnt / fmt_channels) / fmt_sample_rate;

    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    for (;;)
    {

        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

        if (std::chrono::duration_cast<std::chrono::seconds>(end - begin).count() >= 1)
        {
            printf("\r%li / %i secs",std::chrono::duration_cast<std::chrono::seconds>(end - begin).count(),audioTime);
        }
        
        

        if(engine.isFileDone)
        {
            break;
        }
    }

    printf("\nDone.\n");
    


    delete[] engine.file;
    return 0;
}
