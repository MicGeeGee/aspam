#include <iostream>
#include <map>
#include <string>
#include <list>
#include <regex>
#include <vector>
#include <cstdio>
#include <omp.h>
#include "pcre.h"
#include <cmath>
#include <set>

namespace aspam
{
	namespace params
	{
		const std::string regex_pattern_str
			="[\\x21-\\x7E][/!?#]?[-a-zA-Z0-9]*(?:[\"¡¯=;]|/?>|:/*)?";
		const float promotion_factor=1.23;
		const float demotion_factor=0.83;
		const float init_weight=1.0;
		const bool label_spam=1;
		const bool label_ham=0;
		const int cache_size=100001;
		const int win_size=5;
		const int ovec_count=30;
		const int ovec_offset=1;
		const int BKDR_size=501;
		const float thres_thickness=0.05;
		
		
	}


	template<class T>
	class BKDR
	{
	public:
		struct pair
		{
			std::string key;
			T val;
			pair()
			{
			
			}
			pair(const std::string& str):key(str),val()
			{
			
			}
			pair(const std::string& str,const T& value):key(str),val(value)
			{
			
			}
		};

		BKDR()
		{
			num=NULL;
			volume=params::BKDR_size;
			arr=std::vector<std::list<pair> >(volume);
			init_val=NULL;
		}
		BKDR(int vol,T init_v)
		{
			num=NULL;
			volume=vol;
			arr=std::vector<std::list<pair> >(volume);
			init_val=init_v;
		}

		T& operator[] (const std::string& x)
		{
			return find(x)->val;
		}
		int size() const
		{
			return num;
		}
		int vol() const
		{
			return volume;
		}
		bool empty() const
		{
			return !(num>0);
		}

		std::vector<std::list<pair> > arr;

		bool is_existing(const std::string& x)
		{
			int idx=Hash(x.c_str());
			if(arr[idx].empty())
				return false;
			else
			{
				std::list<pair>::iterator it;
				for(it=arr[idx].begin();it!=arr[idx].end();
					it++)
					if(it->key==x)
						return true;
				return false;
			}
		}
	protected:
		

		pair* find(const std::string& x)
		{
			int idx=Hash(x.c_str());
			if(arr[idx].empty())
			{
				arr[idx].push_back(pair(x,init_val));
				num++;
				return &(*(arr[idx].begin()));
			}
			else
			{
				std::list<pair>::iterator it;
				for(it=arr[idx].begin();it!=arr[idx].end();
					it++)
					if(it->key==x)
						return &(*it);
				
				arr[idx].push_front(pair(x,init_val));
				num++;
				return &(*(arr[idx].begin()));
			}
		}

		unsigned int Hash(const char* str)
		{
			unsigned int seed=131;
			unsigned int hash=0;
			while(*str)
				hash=hash*seed+(*str++);

			return (hash&0x7FFFFFFF)%volume;
		}
		
		
		int num;
		int volume;
		T init_val;
	};

	class feature:public BKDR<int>
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

		bool get_label() const
		{
			return this->is_spam;
		}

		void print()
		{
			for(int i=0;i<this->volume;i++)
			{
				std::list<BKDR::pair>::iterator it;
				for(it=this->arr[i].begin();it!=this->arr[i].end();
					it++)
					std::cout<<it->key<<" : "<<it->val<<std::endl;
			}
		}
		void print(const std::string& file_path)
		{
			FILE* fp;
			fp=fopen(file_path.c_str(),"w");
			
			fprintf(fp,"%d\n",this->is_spam);

			for(int i=0;i<this->volume;i++)
			{
				std::list<BKDR::pair>::iterator it;
				for(it=this->arr[i].begin();it!=this->arr[i].end();
					it++)
					fprintf(fp,"%s %d\n",it->key.c_str(),it->val);
					
			}

			fclose(fp);
		}

		void load(const std::string& file_path)
		{
			FILE* fp;
			fp=fopen(file_path.c_str(),"r");

			if(!fp)
			{
				std::cout<<"Error: cannot open "<<file_path<<"."<<std::endl;
				return;
			}
			int res=fscanf(fp,"%d",&this->is_spam);
			if(!(res>0))
			{
				std::cout<<"Error: wrong format."<<std::endl;
				return;
			}
			
			char str[10000];
			int val;
			while (true)
			{
				int res=fscanf(fp,"%s%d",str,&val);
				if(!(res>0))
					break;
				(*this)[str]=val;
			}
			
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
			
			if(!this->is_existing(attr_str))
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

		struct str_ptr
		{
			const char* begin;
			int len;
			str_ptr():begin(NULL),len(NULL)
			{
			}
			str_ptr(const char* begin,int len):begin(begin),len(len)
			{
				
			}
		};


		void extract(const std::string& cnt)
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
					str_ptr(cnt.c_str()+ovector[0],ovector[1]-ovector[0]);
				if(r-f==params::win_size-1)
				{
					for(int i=f;i!=r;i++)
					{
						char str[10000];
						sprintf(str,"%.*s%.*s",str_win[i%params::win_size].len,str_win[i%params::win_size].begin,
							str_win[r%params::win_size].len,str_win[r%params::win_size].begin);
						incre(str);

						/*incre(std::string(str_win[i%params::win_size].begin,str_win[i%params::win_size].len)+
						std::string(str_win[r%params::win_size].begin,str_win[r%params::win_size].len));*/
					}
					f++;
				}
				r++;

				/*str_win[r%params::win_size]=
					std::string(cnt.c_str()+ovector[0],
					ovector[1]-ovector[0]);
				if(r-f==params::win_size-1)
				{
					for(int i=f;i!=r;i++)
						incre(str_win[i%params::win_size]+str_win[r%params::win_size]);
					f++;
				}
				r++;*/

			}
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


	class LSH
	{
	public:
		BKDR<float>& generate_v(std::set<std::string> attr_set)
		{
			
			std::set<std::string>::iterator it;
			for(it=attr_set.begin();it!=attr_set.end();it++)
				v[*it]=gauss_rand();
			
			return v;
		}

	private:
		float gauss_rand()
		{
			static float V1,V2,S;
			static int phase=0;
			float X;

			if(phase==0)
			{
				do
				{
					float U1=(float)rand()/RAND_MAX;
					float U2=(float)rand()/RAND_MAX;

					V1=2*U1-1;
					V2=2*U2-1;
					S=V1*V1+V2*V2;

				}while(S>=1||S==0);

				X=V1*sqrt(-2*log(S)/S);
			}
			else
				X=V2*sqrt(-2*log(S)/S);

			phase=1-phase;

			return X;
		}
		
		BKDR<float> v;

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
				
				float score;
				int num;
				compute_score_and_num(*it,score,num);

				bool label_real=it->get_label();

				if(label_real==params::label_spam)
					if(!(score>(1+params::thres_thickness)*num))
					//1->0(spam->ham):promote
						promote(*it);
				else
					if(!(score<(1-params::thres_thickness)*num))
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

			float score;
			int num;
			compute_score_and_num(ft,score,num);

			bool label_real=ft.get_label();

			if(label_real==params::label_spam)
				if(!(score>(1+params::thres_thickness)*num))
				//1->0(spam->ham):promote
					promote(ft);
			else
				if(!(score<(1-params::thres_thickness)*num))
				//0->1(ham->spam):demote
					demote(ft);

			//bool label_hyp=is_spam(ft);
			//bool label_real=ft.get_label();

			//if(label_hyp!=label_real)
			//	if(label_hyp==params::label_ham)
			//	//1->0(spam->ham):promote
			//		promote(ft);
			//	else
			//	//0->1(ham->spam):demote
			//		demote(ft);
		
		}

		void compute_score_and_num(const feature& ft,float& score,int& num)
		{
			score=0;
			num=0;
			std::map<std::string,int>::const_iterator it;

			for(int i=0;i<ft.vol();i++)
			{
				std::list<BKDR<int>::pair>::const_iterator it;
				for(it=ft.arr[i].begin();it!=ft.arr[i].end();
					it++)
				{
					num+=it->val;
					score+=(it->val)*weights.get(it->key);
				}
			}
		}

		bool is_spam(const feature& ft)
		{
			float sum=0;
			int num=0;
			std::map<std::string,int>::const_iterator it;

			for(int i=0;i<ft.vol();i++)
			{
				std::list<BKDR<int>::pair>::const_iterator it;
				for(it=ft.arr[i].begin();it!=ft.arr[i].end();
					it++)
				{
					num+=it->val;
					sum+=(it->val)*weights.get(it->key);
				}
			}

			

			return sum>num?params::label_spam:params::label_ham;
		}

		void print(const std::string& file_path)
		{
			weights.print(file_path);
		}
		void print()
		{
			weights.print();
		}
		void load(const std::string& file_path)
		{
			weights.load(file_path);
		}

	protected:
		class cache:public BKDR<float>
		{
		public:
			cache():BKDR(params::cache_size,params::init_weight)
			{}

			float get(const std::string& x)
			{
				if(!is_existing(x))
					return params::init_weight;
				return find(x)->val;
			}

			/*float& operator[] (const std::string& x)
			{
				if(!is_existing(x))
					return float(params::init_weight);
				return find(x)->val;
			}*/

			void print()
			{
				for(int i=0;i<this->volume;i++)
				{
					std::list<BKDR::pair>::iterator it;
					for(it=this->arr[i].begin();it!=this->arr[i].end();
						it++)
						std::cout<<it->key<<" : "<<it->val<<std::endl;
				}
			}
			void print(const std::string& file_path)
			{
				FILE* fp;
				fp=fopen(file_path.c_str(),"w");
			
				for(int i=0;i<this->volume;i++)
				{
					std::list<BKDR::pair>::iterator it;
					for(it=this->arr[i].begin();it!=this->arr[i].end();
						it++)
						fprintf(fp,"%s %f\n",it->key.c_str(),it->val);
					
				}

				fclose(fp);
			}

			void load(const std::string& file_path)
			{
				FILE* fp;
				fp=fopen(file_path.c_str(),"r");

				if(!fp)
				{
					std::cout<<"Error: cannot open "<<file_path<<"."<<std::endl;
					return;
				}
				
				
			
				char str[10000];
				float val;
				while (true)
				{
					int res=fscanf(fp,"%s%f",str,&val);
					if(!(res>0))
						break;
					(*this)[str]=val;
				}
			
				fclose(fp);
			}

		}weights;
		


		void promote(const feature& ft)
		{
			for(int i=0;i<ft.vol();i++)
			{
				std::list<BKDR<int>::pair>::const_iterator it;
				for(it=ft.arr[i].begin();it!=ft.arr[i].end();
					it++)
					weights[it->key]*=params::promotion_factor;
			}

			/*std::map<std::string,int>::const_iterator it;
			for(it=ft.begin();it!=ft.end();it++)
				weights[it->first]*=params::promotion_factor;*/
		}
		void demote(const feature& ft)
		{
			for(int i=0;i<ft.vol();i++)
			{
				std::list<BKDR<int>::pair>::const_iterator it;
				for(it=ft.arr[i].begin();it!=ft.arr[i].end();
					it++)
					weights[it->key]*=params::demotion_factor;
			}


			/*std::map<std::string,int>::const_iterator it;
			for(it=ft.begin();it!=ft.end();it++)
				weights[it->first]*=params::demotion_factor;*/
		}
	};


	


}