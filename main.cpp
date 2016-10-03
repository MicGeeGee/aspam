#include "aspam.h"

int main()
{

	aspam::Winnow filter;

	
	/*filter.load("1.txt");

	system("pause");*/

	std::list<aspam::OSB> tra_set;

	for(int i=1;i<=3;i++)
	{
		char ham_str[100];
		char spam_str[100];
		sprintf(ham_str,"data_set//easy_ham_cnt//%d",i);
		sprintf(spam_str,"data_set//spam_cnt//%d",i);
		
		aspam::OSB ham_ft;
		aspam::OSB spam_ft;
		
		ham_ft.extract_and_label(ham_str,aspam::params::label_ham);
		spam_ft.extract_and_label(spam_str,aspam::params::label_spam);

		tra_set.push_back(ham_ft);
		tra_set.push_back(spam_ft);
		//ham_ft.print();

		filter.incre_train(ham_ft);
		filter.incre_train(spam_ft);

		std::cout<<i<<std::endl;
	}
	

	/*filter.print("1.txt");*/

	system("pause");


	return 0;

	//aspam::Winnow filter;

	//std::list<aspam::feature> tra_set;

	//for(int i=1;i<=3;i++)
	//{
	//	char ham_str[100];
	//	char spam_str[100];
	//	sprintf(ham_str,"data_set//easy_ham_cnt//%d",i);
	//	sprintf(spam_str,"data_set//spam_cnt//%d",i);
	//	
	//	aspam::feature ham_ft;
	//	aspam::feature spam_ft;
	//	
	//	ham_ft.extract_and_label(ham_str,aspam::params::label_ham);
	//	spam_ft.extract_and_label(spam_str,aspam::params::label_spam);

	//	tra_set.push_back(ham_ft);
	//	tra_set.push_back(spam_ft);
	//	//ham_ft.print();

	//	filter.incre_train(ham_ft);
	//	filter.incre_train(spam_ft);

	//	std::cout<<i<<std::endl;
	//}

	

	//filter.load_tra_set(&tra_set);
	//filter.train();
	

	
	


	return 0;
}