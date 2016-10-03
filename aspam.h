#include <iostream>
#include <map>
#include <string>
#include <list>
#include <regex>
#include <vector>
#include <cstdio>
#include <omp.h>
#include "pcre.h"

namespace aspam
{
	namespace params
	{
		const std::string regex_pattern_str
			="[\\x21-\\x7E][/!?#]?[-a-zA-Z0-9]*(?:[\"¡¯=;]|/?>|:/*)?";
		const float promotion_factor=1.23;
		const float demotion_factor=0.83;
		const float init_weight=1;
		const bool label_spam=1;
		const bool label_ham=0;
		const int cache_size=500;
		const int win_size=5;
		const int ovec_count=30;
		const int ovec_offset=1;
	}

	class feature:protected std::map<std::string,int>
	{
	public:
		/*class ft_iterator:protected std::map<std::string,int>::iterator
		{
		public:
			std::string fisrt()
			{
				return this->fisrt;
			}
			int& second()
			{
				return this->second;
			}
			ft_iterator operator++(int)
			{
				ft_iterator old(*this);
				(*this)++;
				return old;
			}
		};
*/
		bool empty() const
		{
			return std::map<std::string,int>::empty();
		}

		int& operator[] ( const std::string& x )
		{
			return std::map<std::string,int>::operator[](x);
		}
		int size()
		{
			return std::map<std::string,int>::size();
		}
		std::map<std::string,int>::const_iterator begin() const
		{
			return std::map<std::string,int>::begin();
		}
		std::map<std::string,int>::const_iterator end() const
		{
			return std::map<std::string,int>::end();
		}

		void extract_and_label(const std::string& cnt,bool is_spam)
		{
			extract(cnt);
			label(is_spam);
		}
		void extract_and_label(const char* file_path,bool is_spam)
		{
			extract(file_path);
			label(is_spam);
		}

		bool get_label() const
		{
			return this->is_spam;
		}

		void print()
		{
			std::map<std::string,int>::const_iterator it;
			for(it=this->begin();it!=this->end();it++)
				printf("%s %d\n",it->first.c_str(),it->second);
		}
		void print(const std::string& file_path)
		{
			FILE* fp;
			fp=fopen(file_path.c_str(),"w");
			
			std::map<std::string,int>::const_iterator it;
			for(it=this->begin();it!=this->end();it++)
				fprintf(fp,"%s %d\n",it->first.c_str(),it->second);
			fclose(fp);
		}

	protected:
		void extract(const std::string& cnt)
		{
			pcre* re;
			int ovector[params::ovec_count];
			memset(ovector,0,sizeof(int)*params::ovec_count);
			int rc;
			const char *error;
			int erroffset;
			re = pcre_compile(params::regex_pattern_str.c_str(),
			0, &error, &erroffset, NULL);
			if (re == NULL) {
					printf("PCRE compilation failed at offset %d: %s\n", erroffset, error);
					return;
			}

			while(true)
			{
				rc = pcre_exec(re, NULL, cnt.c_str(), cnt.size(),
				ovector[params::ovec_offset], 0, ovector, 
				params::ovec_count);
				if(rc<0)
				{
					if(rc!=PCRE_ERROR_NOMATCH)
						std::cout<<"Error: cannot match substring"
						<<std::endl;
					free(re);
					return;
				}

				incre(std::string(cnt.c_str()+ovector[0],
					ovector[1]-ovector[0]));
			}

			free(re);

			//std::smatch matcher;
			//std::tr1::regex_constants::syntax_option_type op_type 
			//	= std::tr1::regex_constants::ECMAScript;
			//std::regex pattern(params::regex_pattern_str);
			//
			//const std::sregex_token_iterator end;
			//for(std::sregex_token_iterator i(cnt.begin(),cnt.end(),pattern);
			//	i!=end;i++)
			//	incre(*i);

		}
		void extract(const char* file_path)
		{
			FILE* fp;
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

			extract(cnt);
		}
		void label(bool is_spam)
		{
			this->is_spam=is_spam;
		}	
		
		void incre(const std::string& attr_str)
		{
			if(this->find(attr_str)==this->end())
				(*this)[attr_str]=1;
			else
				(*this)[attr_str]++;
		}
		bool is_spam;

	};


	class OSB:public feature
	{
	public:
		void extract_and_label(const std::string& cnt,bool is_spam)
		{
			extract(cnt);
			label(is_spam);
		}
		void extract_and_label(const char* file_path,bool is_spam)
		{
			extract(file_path);
			label(is_spam);
		}
	private:
		void extract(const std::string& cnt)
		{
			std::vector<std::string> str_win(params::win_size);
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
				rc=pcre_exec(re,NULL,cnt.c_str(),cnt.size(),
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
					std::string(cnt.c_str()+ovector[0],
					ovector[1]-ovector[0]);
				if(r-f==params::win_size-1)
				{
					for(int i=f;i!=r;i++)
						incre(str_win[i%params::win_size]+str_win[r%params::win_size]);
					f++;
				}
				r++;

			
			}


//			std::smatch matcher;
//			std::tr1::regex_constants::syntax_option_type op_type 
//				= std::tr1::regex_constants::ECMAScript;
//			std::regex pattern(params::regex_pattern_str);
//			
//			const std::sregex_token_iterator end;
//
////			omp_set_num_threads(8);
////#pragma opm parallel for
//			for(std::sregex_token_iterator str_i(cnt.begin(),cnt.end(),pattern);
//				str_i!=end;str_i++)
//			{
//				str_win[r%params::win_size]=*str_i;
//				if(r-f==params::win_size-1)
//				{
//					for(int i=f;i!=r;i++)
//						incre(str_win[i%params::win_size]+str_win[r%params::win_size]);
//					f++;
//				}
//				r++;
//			}
		}
		void extract(const char* file_path)
		{
			FILE* fp;
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

			extract(cnt);
		}
	
	};

	class classifier
	{
	public:
		classifier()
		{}
		classifier(std::list<feature>* p_set)
		{
			tra_set=p_set;
		}
		classifier(const classifier& c)
		{
			this->tra_set=c.tra_set;
		}
		void load_tra_set(std::list<feature>* p_set)
		{
			this->tra_set=p_set;
		}
		std::list<feature>* get_tra_set()
		{
			return tra_set;
		}
		virtual bool is_spam(const feature& ft)=0;

	protected:
		std::list<feature>* tra_set;

	};


	class Winnow:public classifier
	{
	public:
		void train()
		{
			if(!this->tra_set)
			{
				std::cout<<"Error: training set is null."<<std::endl;
				return;
			}

			std::list<feature>::iterator it;
			for(it=this->tra_set->begin();it!=this->tra_set->end();it++)
			{
				bool label_hyp=is_spam(*it);
				bool label_real=it->get_label();

				if(label_hyp!=label_real)
					if(label_hyp==params::label_ham)
					//1->0(spam->ham):promote
						promote(*it);
					else
					//0->1(ham->spam):demote
						demote(*it);
			}

		}

		void incre_train(const feature& ft)
		{
			if(ft.empty())
			{
				std::cout<<"Error: the feature is null"<<std::endl;
				return;
			}

			bool label_hyp=is_spam(ft);
			bool label_real=ft.get_label();

			if(label_hyp!=label_real)
				if(label_hyp==params::label_ham)
				//1->0(spam->ham):promote
					promote(ft);
				else
				//0->1(ham->spam):demote
					demote(ft);
		
		}

		bool is_spam(const feature& ft)
		{
			float sum=0;
			int num=0;
			std::map<std::string,int>::const_iterator it;
			for(it=ft.begin();it!=ft.end();it++)
			{
				num+=it->second;
				sum+=(it->second)*weights[it->first];
			}

			return sum>num?params::label_spam:params::label_ham;
		}


	protected:
		class LRU:protected std::map<std::string,float>
		{
		public:
				std::map<std::string,float>::iterator begin()
				{
					return std::map<std::string,float>::begin();
				}
				std::map<std::string,float>::iterator end()
				{
					return std::map<std::string,float>::end();
				}

				float& operator[] (const std::string& x)
				{
					if(this->empty())
					{
						record(x);
						return std::map<std::string,float>::operator[](x)=1;
					}
					std::map<std::string,float>::iterator res=
						this->find(x);
					if(res==this->end())
						if(this->size()==params::cache_size)
						//The cache is fully loaded, 
						//some block should be substituted:
						//And the block in 2nd cache should be reloaded.
							return sub_relo_reco(x);
						else
						{
						//The cache is not fully loaded.
							record(x);
							return std::map<std::string,float>::operator[](x)=1;
						}
					else
					{
					//Successfully found.
						record(x);
						return res->second;
					}
				}
				int size()
				{
					return std::map<std::string,float>::size();
				}
				std::map<std::string,float>::const_iterator begin() const
				{
					return std::map<std::string,float>::begin();
				}
				std::map<std::string,float>::const_iterator end() const
				{
					return std::map<std::string,float>::end();
				}
		private:
				std::map<std::string,int> records;
				std::map<std::string,float> sec_cache;

				void record(const std::string& key)
				{
					std::map<std::string,int>::iterator res=
						records.find(key);
					if(res==records.end())
						records[key]=1;
					else
						res->second++;
				}
				float& sub_relo_reco(const std::string& key)
				{
					int l=records.begin()->second;
					
					std::string l_key=records.begin()->first;//This block should be substituted.
					std::map<std::string,int>::iterator l_it=records.begin();
					
					for(std::map<std::string,int>::iterator it=records.begin();
						it!=records.end();it++)
						if(it->second<l)
						{
							l=it->second;
							l_key=it->first;
							l_it=it;
						}

					std::map<std::string,float>::iterator era_it=
						std::map<std::string,float>::find(l_key);
					float wb_val=era_it->second;

					//Erase block:
					records.erase(l_it);
					std::map<std::string,float>::erase(era_it);

					//Write back:
					sec_cache[l_key]=wb_val;

					//Record:
					records[key]=1;

					//Reloaded from 2nd cache:
					std::map<std::string,float>::iterator res_it=sec_cache.find(key);
					if(res_it==sec_cache.end())
						return (*this)[key]=1;
					else
						return (*this)[key]=res_it->second;
				}

				
		
		}weights;


		void promote(const feature& ft)
		{
			std::map<std::string,int>::const_iterator it;
			for(it=ft.begin();it!=ft.end();it++)
				weights[it->first]*=params::promotion_factor;
		}
		void demote(const feature& ft)
		{
			std::map<std::string,int>::const_iterator it;
			for(it=ft.begin();it!=ft.end();it++)
				weights[it->first]*=params::demotion_factor;
		}
	};






}