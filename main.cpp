#include "aspam.h"

namespace cmd
{
	void train()
	{
		aspam::CMFS::load_res(100000);

		aspam::Winnow filter;

		for(int i=1;i<=500;i++)
		{
			char ham_str[100];
			char spam_str[100];
			sprintf(ham_str,"data_set//easy_ham_cnt//%d",i);
			sprintf(spam_str,"data_set//spam_cnt//%d",i);

			aspam::OSB ham_ft;
			aspam::OSB spam_ft;
		
			/*ham_ft.extract_and_label_with_selection(ham_str,aspam::params::label_ham);
			spam_ft.extract_and_label_with_selection(spam_str,aspam::params::label_spam);
			*/
			ham_ft.extract_and_label(ham_str,aspam::params::label_ham);
			spam_ft.extract_and_label(spam_str,aspam::params::label_spam);


			filter.incre_train(ham_ft);
			filter.incre_train(spam_ft);

			std::cout<<i<<std::endl;
		}
		char filter_str[100];
		sprintf(filter_str,"weights//weights");
		filter.print(filter_str);


		int spam_to_ham=0;
		int ham_to_spam=0;

		for(int i=501;i<=600;i++)
		{
			char ham_str[100];
			char spam_str[100];
			sprintf(ham_str,"data_set//easy_ham_cnt//%d",i);	
			sprintf(spam_str,"data_set//spam_cnt//%d",i);
			
			aspam::OSB ham_ft;
			aspam::OSB spam_ft;
		
					/*ham_ft.extract_and_label_with_selection(ham_str,aspam::params::label_ham);
			spam_ft.extract_and_label_with_selection(spam_str,aspam::params::label_spam);
			*/
			ham_ft.extract_and_label(ham_str,aspam::params::label_ham);
			spam_ft.extract_and_label(spam_str,aspam::params::label_spam);

			bool spam_res;
			bool ham_res;
			ham_res=filter.is_spam(ham_ft);
			spam_res=filter.is_spam(spam_ft);
			if(spam_res!=aspam::params::label_spam)
				spam_to_ham++;
			if(ham_res!=aspam::params::label_ham)
				ham_to_spam++;
			
			printf("Testing sample #%d: spam to ham: %d, ham to spam: %d\n",
				i,spam_to_ham,ham_to_spam);
		}

		exit(0);

	}

	void load()
	{
		aspam::Winnow filter;

		for(int i=1;i<=2500;i++)
		{
			char ham_str[100];
			char spam_str[100];
			sprintf(ham_str,"ft_set//ham//%d",i);
			sprintf(spam_str,"ft_set//spam//%d",i);
			aspam::OSB ham_ft;
			aspam::OSB spam_ft;

			ham_ft.load(ham_str);
			spam_ft.load(spam_str);

			filter.incre_train(ham_ft);
			filter.incre_train(spam_ft);

			std::cout<<i<<std::endl;
		}

		char filter_str[100];
		sprintf(filter_str,"weights//weights");
		filter.print(filter_str);
	
	}

	void load_filter()
	{
		aspam::Winnow filter;
		char filter_str[100];
		sprintf(filter_str,"weights//weights");
		filter.load(filter_str);
	}

	void train_bagging()
	{
		aspam::CMFS::load_res(100000);

		aspam::bagging filter;
		filter.train_with_selection("data_set//spam_20021010","data_set//ham_20021010",1,500);
		filter.save();
		filter.print_records();
		filter.test_raw_samples("data_set//spam_20021010","data_set//ham_20021010",1,500);

		
		exit(0);
	}

	void construct_LSH()
	{
		aspam::LSH a;

		for(int i=1;i<=2500;i++)
		{
			char ham_str[100];
			char spam_str[100];
			sprintf(ham_str,"data_set//easy_ham_cnt//%d",i);
			sprintf(spam_str,"data_set//spam_cnt//%d",i);
		
			aspam::OSB ham_ft;
			aspam::OSB spam_ft;
		
			ham_ft.extract_and_label(ham_str,aspam::params::label_ham);
			spam_ft.extract_and_label(spam_str,aspam::params::label_spam);

			a.construct(ham_ft,i);
			a.construct(spam_ft,i);
			



			std::cout<<i<<std::endl;
		}
	
	}

	void load_bagging()
	{
		aspam::bagging filter;
		filter.load();
	}


	void concept_drift()
	{
		aspam::Winnow filter;
		char filter_str[100];
		sprintf(filter_str,"weights//weights");
		filter.load(filter_str);


		for(int i=1;i<=400;i++)
		{
			char ham_str[100];
			char spam_str[100];
			sprintf(ham_str,"data_set//easy_ham//%d",i);
			sprintf(spam_str,"data_set//spam//%d",i);
		
			aspam::OSB ham_ft;
			aspam::OSB spam_ft;
		
			ham_ft.extract_and_label(ham_str,aspam::params::label_ham);
			spam_ft.extract_and_label(spam_str,aspam::params::label_spam);

			filter.incre_train(ham_ft);
			filter.incre_train(spam_ft);

			std::cout<<i<<std::endl;
		}

		int ham_to_spam=0;
		int spam_to_ham=0;
		for(int i=401;i<=480;i++)
		{
			char ham_str[100];
			char spam_str[100];
			//sprintf(ham_str,"data_set//easy_ham_cnt//%d",i);
			//sprintf(spam_str,"data_set//spam_cnt//%d",i);
			sprintf(ham_str,"data_set//easy_ham//%d",i);
			sprintf(spam_str,"data_set//spam//%d",i);
		

			aspam::OSB ham_ft;
			aspam::OSB spam_ft;
		
			ham_ft.extract_and_label(ham_str,aspam::params::label_ham);
			spam_ft.extract_and_label(spam_str,aspam::params::label_spam);

	

			if(filter.is_spam(ham_ft))
				ham_to_spam++;
			if(!filter.is_spam(spam_ft))
				spam_to_ham++;

			printf("%d : h_to_s %d, spam_to_ham %d\n",i,ham_to_spam,spam_to_ham);
		}
		
	}

	void feature_selection()
	{
		aspam::Winnow filter;
		aspam::CMFS selector;

		for(int i=1;i<=500;i++)
		{
			char ham_str[100];
			char spam_str[100];
			sprintf(ham_str,"data_set//ham_20021010//%d",i);
			sprintf(spam_str,"data_set//spam_20021010//%d",i);
		
			/*sprintf(ham_str,"data_set//easy_ham_cnt//%d",i);	
			sprintf(spam_str,"data_set//spam_cnt//%d",i);
			*/

			aspam::OSB ham_ft;
			aspam::OSB spam_ft;
		
			ham_ft.extract_and_label(ham_str,aspam::params::label_ham);
			spam_ft.extract_and_label(spam_str,aspam::params::label_spam);


			selector.counting(ham_ft);
			selector.counting(spam_ft);

	
			std::cout<<i<<std::endl;
		}
	
		selector.selecting_ft(100000000);

		/*char selector_str[100];
		sprintf(selector_str,"CMFS//selected_fts");
		selector.print(selector_str);*/



		system("pause");
		
		exit(0);
	}

}


int main()
{
	//cmd::load_filter();
	cmd::train();
	//cmd::train_bagging();
	//cmd::construct_LSH();
	//cmd::load_bagging();

	//cmd::feature_selection();

	/*aspam::bagging filter;
	filter.load();
	filter.load_records();
	filter.test_ft("ft_set//spam","ft_set//ham",1,2500);
	*/
	
	exit(0);

	aspam::Winnow filter;

	char ham_str[100];
	char spam_str[100];
	sprintf(ham_str,"data_set//easy_ham_cnt//%d",359);
	sprintf(spam_str,"data_set//spam_cnt//%d",359);

	aspam::OSB ham_ft;
	aspam::OSB spam_ft;
		
	/*ham_ft.extract_and_label_with_selection(ham_str,aspam::params::label_ham);
	spam_ft.extract_and_label_with_selection(spam_str,aspam::params::label_spam);
	*/
	ham_ft.extract_and_label(ham_str,aspam::params::label_ham);
	spam_ft.extract_and_label(spam_str,aspam::params::label_spam);


	filter.incre_train(ham_ft);
	filter.incre_train(spam_ft);

	
		





	exit(0);

	return 0;
}