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
#include <queue>

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
		const int ensemble_num=25;
		const std::string classifier_root="weights";
		const std::string ft_root="ft_set";
		const float init_base_weight=1.0;
		const float spam_thres=0.5;
		const int k=5;
		const int L=5;
		const int seg_len=21;
		const int LSH_vol=501;
		const std::string LSH_root="LSH";
		const int spam_idx_factor=1;
		const int ham_idx_factor=-1;
		const int nn_num=31;
		const float sample_prob=0.68;

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

		pair* find(const std::string& x,T init)
		{
			int idx=Hash(x.c_str());
			if(arr[idx].empty())
			{
				arr[idx].push_back(pair(x,init));
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
				
				arr[idx].push_front(pair(x,init));
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

		float product(BKDR<float>& x) const
		{
			float res=0;
			for(int m=0;m<this->volume;m++)
			{
				std::list<BKDR::pair>::const_iterator it;
				for(it=this->arr[m].begin();it!=this->arr[m].end();
					it++)
					//res+=x[it->key]*(it->val);
					res+=fast_product(it->val,x[it->key]);
			}
			return res;
		}
		float product(BKDR<int>& x) const
		{
			float res=0;
			for(int m=0;m<this->volume;m++)
			{
				std::list<BKDR::pair>::const_iterator it;
				for(it=this->arr[m].begin();it!=this->arr[m].end();
					it++)
					res+=x[it->key]*(it->val);
			}
			return res;
		}

		float dis_sq(BKDR<int>& x) const
		{
			float res=0;
			for(int m=0;m<this->volume;m++)
			{
				std::list<BKDR::pair>::const_iterator it;
				for(it=this->arr[m].begin();it!=this->arr[m].end();
					it++)
				{
					float val=(x[it->key]-(it->val));
					res+=val*val;
				}
			}
			return res;
		}

	protected:

		float fast_product(int x,float y) const
		{
			float res=0;
			for(int i=0;i<x;i++)
				res+=y;
			return res;
		}

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

		class vec:public BKDR<float>
		{
		public:
			float& operator[] (const std::string& x)
			{
				return find(x,gauss_rand())->val;
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
				int val;
				while (true)
				{
					int res=fscanf(fp,"%s%f",str,&val);
					if(!(res>0))
						break;
					(*this)[str]=val;
				}
			
				fclose(fp);
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

				return abs(X);
			}
			
		};

		LSH()
		{
			init(params::L,params::k);
		}

		void init(int L,int k)
		{
			vecs=std::vector<std::vector<vec > >(L);
			bias_terms=std::vector<std::vector<float> >(L);
			tables=std::vector<std::vector<std::list<int> > >(L);

			for(int i=0;i<L;i++)
			{
				vecs[i]=std::vector<vec>(k);
				bias_terms[i]=std::vector<float>(k);
				tables[i]=std::vector<std::list<int> >(params::LSH_vol);

				for(int j=0;j<k;j++)
					bias_terms[i][j]=uni_rand();
			}
		}

		std::vector<std::vector<int> > project(const feature& ft)
		{
			std::vector<std::vector<int> > res(params::L);
			
			for(int i=0;i<params::L;i++)
				res[i]=std::vector<int>(params::k);
			
			
			for(int i=0;i<params::L;i++)
			{
				for(int j=0;j<params::k;j++)
				{
						/*float sum=0;
						for(int m=0;m<ft.vol();m++)
						{
							std::list<BKDR<int>::pair>::const_iterator it;
							for(it=ft.arr[m].begin();it!=ft.arr[m].end();
								it++)
								sum+=vecs[i][j][it->key]*(it->val);
						}*/
						
						//res[i][j]=(sum+bias_terms[i][j])/params::seg_len;
						res[i][j]=(ft.product(vecs[i][j])+bias_terms[i][j])/params::seg_len;

						//std::cout<<j<<std::endl;
				}
				
			}

			return res;
		}

		int hash(const std::vector<int>& projection)
		{
			int res=0;
			unsigned int seed=131;
			unsigned int hash=0;

			std::vector<int>::const_iterator it;
			for(it=projection.begin();it!=projection.end();
				it++)
				hash=hash*seed+*it;
				
			return (hash&0x7FFFFFFF)%params::LSH_vol;
		}

		void construct(const feature& ft,int idx)
		{
			std::vector<std::vector<int> > res=project(ft);
			for(int i=0;i<params::L;i++)
			{
				int hash_res=hash(res[i]);
				
				if(ft.get_label()==params::label_spam)
					tables[i][hash_res].push_back(params::spam_idx_factor*idx);
				else
					tables[i][hash_res].push_back(params::spam_idx_factor*idx);
			}
		}

		struct pair
		{
			int idx;
			float dis;
			pair(int idx,float dis):idx(idx),dis(dis)
			{
			}
			friend bool operator<(const pair& a,const pair& b)
			{
				  return a.dis>b.dis;
			}               
		};

		std::set<pair> nearest_neighbors(const feature& ft,int k)
		{
			std::vector<std::vector<int> > proj_res=project(ft);
			std::priority_queue<pair> heap;
			for(int i=0;i<params::L;i++)
			{
				int hash_res=hash(proj_res[i]);
				std::list<int>::const_iterator it;
				for(it=tables[i][hash_res].begin();it!=tables[i][hash_res].end();
					it++)
				{
					feature it_ft;
					char ft_str[100];
					sprintf(ft_str,"%s//%s//%d",params::ft_root.c_str(),
						((*it)*params::ham_idx_factor)>0?"ham":"spam",abs(*it));
					it_ft.load(ft_str);
					heap.push(pair((*it),ft.dis_sq(it_ft)));
				}
			}

			std::set<pair> res;
			for(int i=0;i<k && !heap.empty();i++)
			{
				res.insert(heap.top());
				heap.pop();
			}

			return res;
		}

		std::set<pair> nearest_neighbors(const feature& ft,int k,
			std::map<int,feature>& ft_cache)
		{
			std::vector<std::vector<int> > proj_res=project(ft);
			std::priority_queue<pair> heap;
			for(int i=0;i<params::L;i++)
			{
				int hash_res=hash(proj_res[i]);
				std::list<int>::const_iterator it;
				for(it=tables[i][hash_res].begin();it!=tables[i][hash_res].end();
					it++)
				{
					feature it_ft;
					char ft_str[100];
					sprintf(ft_str,"%s//%s//%d",params::ft_root.c_str(),
						((*it)*params::ham_idx_factor)>0?"ham":"spam",abs(*it));
					it_ft.load(ft_str);
					
					ft_cache[*it]=it_ft;

					heap.push(pair((*it),ft.dis_sq(it_ft)));
				}
			}

			std::set<pair> res;
			for(int i=0;i<k && !heap.empty();i++)
			{
				res.insert(heap.top());
				heap.pop();
			}

			return res;
		}

		void save()
		{
			FILE* fp;
			char LSH_str[100];
			sprintf(LSH_str,"%s//params",params::LSH_root.c_str());
			fp=fopen(LSH_str,"w");
			fprintf(fp,"%d %d",params::L,params::k);
			fclose(fp);

			for(int i=0;i<params::L;i++)
				for(int j=0;j<params::k;j++)
				{
					sprintf(LSH_str,"%s//v_%d_%d",params::LSH_root.c_str(),i,j);
					vecs[i][j].print(LSH_str);
				}	
			
			sprintf(LSH_str,"%s//bias_terms",params::LSH_root.c_str());
			fp=fopen(LSH_str,"w");
			for(int i=0;i<params::L;i++)
				for(int j=0;j<params::k;j++)
					fprintf(fp,"%d %d %f\n",i,j,bias_terms[i][j]);
			fclose(fp);
			
			sprintf(LSH_str,"%s//tables",params::LSH_root.c_str());
			fp=fopen(LSH_str,"w");
			for(int i=0;i<params::L;i++)
				for(int j=0;j<params::k;j++)
				{
					std::list<int>::iterator it;
					for(it=tables[i][j].begin();it!=tables[i][j].end();
						it++)
						fprintf(fp,"%d %d %d\n",i,j,*it);
				}
			fclose(fp);
		}
		void load()
		{

			int L;
			int k;
			int x;
			int y;

			FILE* fp;
			char LSH_str[100];
			sprintf(LSH_str,"%s//params",params::LSH_root.c_str());
			fp=fopen(LSH_str,"r");
			fscanf(fp,"%d%d",&L,&k);
			fclose(fp);
			
			init(L,k);

			for(int i=0;i<params::L;i++)
				for(int j=0;j<params::k;j++)
				{
					sprintf(LSH_str,"%s//v_%d_%d",params::LSH_root.c_str(),i,j);
					vecs[i][j].load(LSH_str);
				}	
			
			sprintf(LSH_str,"%s//bias_terms",params::LSH_root.c_str());
			fp=fopen(LSH_str,"r");
			for(int i=0;i<params::L;i++)
				for(int j=0;j<params::k;j++)
					fscanf(fp,"%d%d%f",&x,&y,&bias_terms[i][j]);
			fclose(fp);
			
			sprintf(LSH_str,"%s//tables",params::LSH_root.c_str());
			fp=fopen(LSH_str,"r");
			for(int i=0;i<params::L;i++)
				for(int j=0;j<params::k;j++)
				{
					std::list<int>::iterator it;
					for(it=tables[i][j].begin();it!=tables[i][j].end();
						it++)
						fscanf(fp,"%d%d%d",&x,&y,&(*it));
				}
			fclose(fp);
		}

	private:
		
		std::vector<std::vector<vec > > vecs;
		std::vector<std::vector<float> > bias_terms;
		std::vector<std::vector<std::list<int> > > tables;

		

		float uni_rand()
		{
			float ratio=rand()/(RAND_MAX+0.0);
			return ratio*params::seg_len;
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
		virtual void incre_train(const feature& ft)=0;

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
		std::list<feature>* tra_set;
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
					num+=it->val>0;
					score+=(it->val>0)*weights.get(it->key);
				}
			}
		}

		bool is_spam(const feature& ft)
		{
			float sum=0;
			int num=0;
			
			compute_score_and_num(ft,sum,num);
			return sum>num?params::label_spam:!params::label_spam;
		}

		
		

	protected:
		


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


	class bagging:public classifier
	{
	public:
		void train(const std::string& root_spam,const std::string& root_ham,
			int intrvl_l,int intrvl_r)
		{
			int sample_num=intrvl_r-intrvl_l+1;

			int sub_dset_num=sample_num/params::ensemble_num;

			char ham_str[100];
			char spam_str[100];
			char filter_str[100];

			bases=std::vector<Winnow>(params::ensemble_num);
			bases_records=std::vector<std::set<int> >(params::ensemble_num);
			bases_weights=std::vector<float>(params::ensemble_num);

			for(int i=0;i<params::ensemble_num;i++)
			{
				bases_weights[i]=params::init_base_weight;

				for(int j=0;j<sub_dset_num;j++)
				{
					int idx=generate_rand(intrvl_l,intrvl_r);
					records.insert(idx);

					if(bases_records[i].find(idx)!=bases_records[i].end())
						continue;
					else
						bases_records[i].insert(idx);
					

					sprintf(ham_str,"%s//%d",root_ham.c_str(),idx);
					sprintf(spam_str,"%s//%d",root_spam.c_str(),idx);
		
					aspam::OSB ham_ft;
					aspam::OSB spam_ft;

					ham_ft.extract_and_label(ham_str,params::label_ham);
					spam_ft.extract_and_label(spam_str,params::label_spam);

					bases[i].incre_train(ham_ft);
					bases[i].incre_train(spam_ft);

					printf("Training base %d using %d (%d/%d).\n",i,idx,j+1,sub_dset_num);
				}
				sprintf(filter_str,"%s//%d",
					params::classifier_root.c_str(),i);

				bases[i].print(filter_str);
			}
		}
		void save()
		{
			char filter_str[100];
			for(int i=0;i<params::ensemble_num;i++)
			{
				sprintf(filter_str,"%s//%d",
					params::classifier_root.c_str(),i);
				bases[i].print(filter_str);
			}

			FILE* fp;
			sprintf(filter_str,"%s//base_weights",
					params::classifier_root.c_str());
			fp=fopen(filter_str,"w");
			fprintf(fp,"%d\n",params::ensemble_num);
			for(int i=0;i<params::ensemble_num;i++)
				fprintf(fp,"%d %f\n",i,bases_weights[i]);
			fclose(fp);
		}
		void load()
		{
			char filter_str[100];

			FILE* fp;
			sprintf(filter_str,"%s//base_weights",
					params::classifier_root.c_str());
			fp=fopen(filter_str,"r");
			int ensumble_num;
			int x;
			fscanf(fp,"%d",&ensumble_num);
			bases=std::vector<Winnow>(ensumble_num);
			//bases_records=std::vector<std::set<int> >(ensumble_num);
			bases_weights=std::vector<float>(ensumble_num);

			for(int i=0;i<ensumble_num;i++)
				fscanf(fp,"%d %f\n",&x,&bases_weights[i]);
			fclose(fp);


			
			for(int i=0;i<ensumble_num;i++)
			{
				sprintf(filter_str,"%s//%d",
					params::classifier_root.c_str(),i);
				std::cout<<"Loading base #"<<i<<std::endl;
				bases[i].load(filter_str);
				
			}
		}

		void print_records()
		{
			FILE* fp;
			char records_str[100];
			sprintf(records_str,"%s//records",params::classifier_root.c_str());
			fp=fopen(records_str,"w");
			std::set<int>::iterator it;
			for(it=records.begin();it!=records.end();it++)
				fprintf(fp,"%d\n",*it);
			fclose(fp);
		}

		void load_records()
		{
			FILE* fp;
			char records_str[100];
			sprintf(records_str,"%s//records",params::classifier_root.c_str());
			fp=fopen(records_str,"r");

			while(true)
			{
				int idx;
				if(!(fscanf(fp,"%d",&idx)>0))
					break;
				records.insert(idx);
			}
			fclose(fp);
		}

		
		void test_ft(const std::string& root_spam,const std::string& root_ham,
			int intrvl_l,int intrvl_r)
		{
			char ham_str[100];
			char spam_str[100];

			int num=0;
			int ham_to_spam=0;
			int spam_to_ham=0;

			for(int i=intrvl_l;i<=intrvl_r;i++)
			{
				if(records.find(i)!=records.end())
					continue;

				sprintf(ham_str,"%s//%d",root_ham.c_str(),i);
				sprintf(spam_str,"%s//%d",root_spam.c_str(),i);
				
				aspam::OSB ham_ft;
				aspam::OSB spam_ft;

				ham_ft.load(ham_str);
				spam_ft.load(spam_str);

				
				if(this->is_spam(ham_ft))
					ham_to_spam++;
				
				if(!this->is_spam(spam_ft))
					spam_to_ham++;
				
				num++;

				printf("Test #%d : %d ham_to_spam(s), %d spam_to_ham(s)\n",num,ham_to_spam,spam_to_ham);
			}
		}


		void test_raw_samples(const std::string& root_spam,const std::string& root_ham,
			int intrvl_l,int intrvl_r)
		{
			char ham_str[100];
			char spam_str[100];

			int num=0;
			int ham_to_spam=0;
			int spam_to_ham=0;

			for(int i=intrvl_l;i<=intrvl_r;i++)
			{
				if(records.find(i)!=records.end())
					continue;

				sprintf(ham_str,"%s//%d",root_ham.c_str(),i);
				sprintf(spam_str,"%s//%d",root_spam.c_str(),i);
				
				aspam::OSB ham_ft;
				aspam::OSB spam_ft;

				ham_ft.extract_and_label(ham_str,params::label_ham);
				spam_ft.extract_and_label(spam_str,params::label_spam);

				if(this->is_spam(ham_ft))
					ham_to_spam++;
				
				if(!this->is_spam(spam_ft))
					spam_to_ham++;

				num++;
				printf("Test #%d : %d ham_to_spam(s), %d spam_to_ham(s)\n",num,ham_to_spam,spam_to_ham);
			}
		}


		bool is_spam(const feature& ft)
		{
			int ham_supporter=0;
			int spam_supporter=0;
			for(int i=0;i<params::ensemble_num;i++)
			{
				if(bases[i].is_spam(ft))
					spam_supporter++;
				else
					ham_supporter++;
			}
			
			return spam_supporter>ham_supporter;
		}


		/*bool is_spam(const feature& ft)
		{
			float sum=0;
			float denom=0;



			for(int i=0;i<params::ensemble_num;i++)
			{	
				float score;
				int num;
				bases[i].compute_score_and_num(ft,score,num);
				sum+=score/num*bases_weights[i];
			}
			for(int i=0;i<bases_weights.size();i++)
				denom+=bases_weights[i];

			return sum/denom>1;
		}*/

		/*bool is_spam(const feature& ft)
		{
			float score=0;
			float denom=0;
			for(int i=0;i<params::ensemble_num;i++)
				score+=bases[i].is_spam(ft)*bases_weights[i];
			
			for(int i=0;i<bases_weights.size();i++)
				denom+=bases_weights[i];

			return score/denom>params::spam_thres;
		}*/
		void incre_train_drift(const feature& ft)
		{
			std::vector<std::vector<int> > proj_res=
				knn_finder.project(ft);
			std::map<int,feature> ft_cache;

			std::set<LSH::pair> knn_res=knn_finder.nearest_neighbors
				(ft,params::nn_num,ft_cache);
			
			for(int i=0;i<bases.size();i++)
			{
				float weight=0;
				float denom=0;
				float nume=0;
				std::set<LSH::pair>::const_iterator it;
				for(it=knn_res.begin();it!=knn_res.end();it++)
				{
					denom+=it->dis;
					int mr;
					if(bases[i].is_spam(ft_cache[it->idx])==params::label_ham)
					{
						if(it->idx*params::ham_idx_factor>0)
							mr=1;
						else
							mr=-1;
					}
					else
					{
						if(it->idx*params::spam_idx_factor>0)
							mr=1;
						else
							mr=-1;
					}
					nume+=(it->dis)*mr;
				}
				bases_weights[i]=nume/denom;
			}


		}

		void incre_train(const feature& ft)
		{
			float x=rand()/(RAND_MAX+0.0);
			for(int i=0;i<bases.size();i++)
				if(x>params::sample_prob)
					continue;
				else
					bases[i].incre_train(ft);
		}

	private:
		int generate_rand(int intrvl_l,int intrvl_r)
		{
			float ratio=rand()/(RAND_MAX+0.0);

			return ratio*(intrvl_r-intrvl_l)+intrvl_l;
		}
		std::vector<Winnow> bases;
		std::vector<std::set<int> > bases_records;
		std::vector<float> bases_weights;
		std::set<int> records;
		LSH knn_finder;
	};
	


}