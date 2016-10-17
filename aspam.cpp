#include "aspam.h"

namespace aspam
{

	census_mtrx CMFS::res;

	void OSB::regex_matching(const char* cnt,int size)
	{
		std::vector<str_ptr> str_win(params::win_size);
		int f=0;
		int r=0;

			
		pcre* re;
		int ovector[params::ovec_count];
		memset(ovector,0,sizeof(int)*params::ovec_count);
		int rc;
		const char *error;
		int erroffset;
		re=pcre_compile(params::regex_pattern_str.c_str(),
			NULL,&error,&erroffset,NULL);
		if(re==NULL)
		{
			std::cout<<"Error: PCRE compilation at offset "<<
				erroffset<<", "<<error<<std::endl;
			return;
		}
		while(true)
		{
			rc=pcre_exec(re,NULL,cnt,size,
				ovector[params::ovec_offset],0,ovector,
				params::ovec_count);
			if(rc<0)
			{
				if(rc!=PCRE_ERROR_NOMATCH)
					std::cout<<"Error: cannot match substring"
					<<std::endl;
				free(re);
				return;
			}

				
			str_win[r%params::win_size]=
				str_ptr(cnt+ovector[0],ovector[1]-ovector[0]);
			if(r-f==params::win_size-1)
			{
				for(int i=f;i!=r;i++)
				{
					char str[10000];
					sprintf(str,"%.*s %.*s",str_win[i%params::win_size].len,str_win[i%params::win_size].begin,
						str_win[r%params::win_size].len,str_win[r%params::win_size].begin);
					incre(str);

				}
				f++;
			}
			r++;


		}
	}

	void OSB::regex_matching(const char* cnt,int size,const census_mtrx& CMFS_res)
	{
		std::vector<str_ptr> str_win(params::win_size);
		int f=0;
		int r=0;

			
		pcre* re;
		int ovector[params::ovec_count];
		memset(ovector,0,sizeof(int)*params::ovec_count);
		int rc;
		const char *error;
		int erroffset;
		re=pcre_compile(params::regex_pattern_str.c_str(),
			NULL,&error,&erroffset,NULL);
		if(re==NULL)
		{
			std::cout<<"Error: PCRE compilation at offset "<<
				erroffset<<", "<<error<<std::endl;
			return;
		}
		while(true)
		{
			rc=pcre_exec(re,NULL,cnt,size,
				ovector[params::ovec_offset],0,ovector,
				params::ovec_count);
			if(rc<0)
			{
				if(rc!=PCRE_ERROR_NOMATCH)
					std::cout<<"Error: cannot match substring"
					<<std::endl;
				free(re);
				return;
			}

				
			str_win[r%params::win_size]=
				str_ptr(cnt+ovector[0],ovector[1]-ovector[0]);
			if(r-f==params::win_size-1)
			{
				for(int i=f;i!=r;i++)
				{
					char str[10000];
					sprintf(str,"%.*s %.*s",str_win[i%params::win_size].len,str_win[i%params::win_size].begin,
						str_win[r%params::win_size].len,str_win[r%params::win_size].begin);

					if(CMFS_res.is_existing(str))
						incre(str);
				}
				f++;
			}
			r++;


		}
	}

	void OSB::extract(const char* file_path)
	{
		CkEmail e;
		if(e.LoadEml(file_path)!=true)
		{
			std::cout<<e.lastErrorText()<<"\r\n";
			return;
		}
		CkString cnt;
		//e.get_Body(cnt);
		e.get_Header(cnt);


		//regex_matching(cnt.getString(),cnt.getSizeUtf8());

		std::string cnt_html_removed(cnt);
		normalizemime::remove_html(cnt_html_removed);

		normalizemime::strtolower(cnt_html_removed);

		/*std::transform(cnt_html_removed.begin(),
				cnt_html_removed.end(),cnt_html_removed.begin(),std::tolower);
*/
		regex_matching(cnt_html_removed.c_str(),cnt_html_removed.size());





		/*FILE* fp;
		fp=fopen(file_path,"r");
		std::string cnt;
		char str[100];
			
		if (fp==NULL) 
			perror ("Error opening file");
		else
		{
			while(true)
			{
				if(fgets(str,100,fp)!=NULL)
				{
					cnt.append(str);		
				}
				else
					break;
					
			} 
		}
		fclose(fp);

		extract(cnt);*/
	}

	void OSB::extract_with_selection(const char* file_path)
	{
		CkEmail e;
		if(e.LoadEml(file_path)!=true)
		{
			std::cout<<e.lastErrorText()<<"\r\n";
			return;
		}
		CkString cnt;
		//e.get_Body(cnt);
		e.get_Header(cnt);


		//regex_matching(cnt.getString(),cnt.getSizeUtf8());

		std::string cnt_html_removed(cnt);
		normalizemime::remove_html(cnt_html_removed);

		normalizemime::strtolower(cnt_html_removed);

		regex_matching(cnt_html_removed.c_str(),cnt_html_removed.size(),CMFS::res);

	
	}

	

	

}