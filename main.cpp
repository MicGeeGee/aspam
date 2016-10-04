#include "aspam.h"

namespace cmd
{
	void train()
	{
		aspam::Winnow filter;

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

			sprintf(ham_str,"ft_set//ham//%d",i);
			sprintf(spam_str,"ft_set//spam//%d",i);

			ham_ft.print(ham_str);
			spam_ft.print(spam_str);

		//	tra_set.push_back(ham_ft);
		//	tra_set.push_back(spam_ft);
	

			filter.incre_train(ham_ft);
			filter.incre_train(spam_ft);

			std::cout<<i<<std::endl;
		}
	

		char filter_str[100];
		sprintf(filter_str,"weights//weights");
		filter.print(filter_str);
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

}


int main()
{
	



	//cmd::load_filter();
	cmd::train();



	system("pause");

	return 0;
}