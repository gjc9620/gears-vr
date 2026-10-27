// Exact frozen31 source-check algorithm, Host comparison only. No game bytes.
bool legacy_source_bytes(bool installed){using namespace live;
    for(auto const&c:cert::chunks){BCRYPT_ALG_HANDLE a{};BCRYPT_HASH_HANDLE h{};unsigned char digest[32]{};std::array<unsigned char,1024> bytes{};
        bool ok=counted_open(&a,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0;
        if(ok)ok=BCryptCreateHash(a,&h,nullptr,0,nullptr,0,0)>=0;
        for(unsigned at=0;ok&&at<c.size;){unsigned n=(std::min)(unsigned(bytes.size()),c.size-at);
            if(installed&&c.rva==cert::entry&&at==0){n=32;ok=own_entry();if(ok)std::copy(cert::prefix.begin(),cert::prefix.end(),bytes.begin());}
            else ok=safe_copy(bytes.data(),reinterpret_cast<void const*>(base+c.rva+at),n);
            if(ok)ok=counted_hash(h,bytes.data(),n,0)>=0;at+=n;}
        if(ok)ok=BCryptFinishHash(h,digest,32,0)>=0;if(h)BCryptDestroyHash(h);if(a)BCryptCloseAlgorithmProvider(a,0);
        constexpr char hex[]="0123456789abcdef";if(ok)for(unsigned i=0;i<32;++i)if(c.sha256[i*2]!=hex[digest[i]>>4]||c.sha256[i*2+1]!=hex[digest[i]&15])ok=false;
        if(!ok)return false;
    }return true;
}
bool legacy_code_current(){using namespace live;std::array<unsigned char,5>b{};return certified&&own_entry()&&legacy_source_bytes(true)&&
    safe_copy(b.data(),reinterpret_cast<void const*>(base+cert::call_rva),5)&&b==cert::call;}
