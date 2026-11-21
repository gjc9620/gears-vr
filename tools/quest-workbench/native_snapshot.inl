    // Explicit one-shot development capture. Draws into a private offscreen target;
    // does not inject input, alter tracking, save poses or change the headset image.
    void capture_ui_if_requested(){
        if(testing_||storage_.pending()||!fs::is_regular_file(data_+"/capture-ui.request"))return;
        fs::remove(data_+"/capture-ui.request");auto old_page=page_;auto old_options=practice_options_;auto old_buttons=buttons_;bool old_advanced=advanced_open_;bool old_open=inspector_open_;auto old_inspector=inspector_;auto old_practice=practice_;bool old_known=practice_known_;
        GLuint target{},color{},depth{};glGenFramebuffers(1,&target);glBindFramebuffer(GL_FRAMEBUFFER,target);
        glGenTextures(1,&color);glBindTexture(GL_TEXTURE_2D,color);glTexStorage2D(GL_TEXTURE_2D,1,GL_RGBA8,1280,1280);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,color,0);
        glGenRenderbuffers(1,&depth);glBindRenderbuffer(GL_RENDERBUFFER,depth);glRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH_COMPONENT24,1280,1280);glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER,depth);
        if(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE){
            for(unsigned capture=0;capture<6;++capture){unsigned page=capture==0?5:capture==1?0:capture==2?0:capture==3?2:1;
                page_=page;practice_options_=capture>=4?capture-3:0;advanced_open_=capture==2;inspector_open_=false;build_ui();refresh_thumbnails();renderer_.begin_scene();glViewport(0,0,1280,1280);glClearColor(.025f,.04f,.065f,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
                auto camera=compose(panel_,{{0,-.225,.95},{}});auto vp=multiply(projection(-.46,.46,-.46,.46),matrix(inverse(camera)));render(vp);renderer_.flush(vp);
                std::vector<unsigned char> pixels(1280*1280*4);glReadPixels(0,0,1280,1280,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());auto error=glGetError();log("ui_snapshot page="+std::to_string(page)+" gl="+std::to_string(error));
                static char const* names[]={"weapons","capture","advanced","library","feedback","shoulder"};if(!error){std::ofstream f(data_+"/debug-ui-"+names[capture]+".ppm",std::ios::binary);f<<"P6\n1280 1280\n255\n";std::vector<char> row(1280*3);for(int y=1279;y>=0;--y){for(unsigned x=0;x<1280;++x)for(unsigned c=0;c<3;++c)row[x*3+c]=char(pixels[(y*1280+x)*4+c]);f.write(row.data(),row.size());}}
            }
        }else log("ui_snapshot framebuffer incomplete");
        advanced_open_=old_advanced;page_=old_page;practice_options_=old_options;practice_=std::move(old_practice);practice_known_=old_known;buttons_=std::move(old_buttons);inspector_open_=old_open;inspector_=old_inspector;glBindFramebuffer(GL_FRAMEBUFFER,0);glDeleteFramebuffers(1,&target);glDeleteTextures(1,&color);glDeleteRenderbuffers(1,&depth);
    }
