

#include "belr/belr.hh"
#include "belr/parser.hh"
#include <algorithm>
#include <iostream>
#include <fstream>
#include <boost/tokenizer.hpp>
#include <string>
#include <vector>
#include <stdio.h>
#include <sstream>
#include <cstring>

using namespace std;
using namespace belr;

std::string parseStr(std::string s, long &strlPos){
  size_t pos = s.find(" ",strlPos);
  std::string ns = s.substr(strlPos, pos-strlPos);
  strlPos  = pos+1;
	return ns;
}



TransitionMap::TransitionMap(){
	for(size_t i=0;i<sizeof(mPossibleChars)/sizeof(bool);++i)
		mPossibleChars[i]=false;
}

bool TransitionMap::intersect(const TransitionMap* other){
	for(size_t i=0;i<sizeof(mPossibleChars)/sizeof(bool);++i){
		if (mPossibleChars[i] && other->mPossibleChars[i]) return true;
	}
	return false;
}

bool TransitionMap::intersect(const TransitionMap *other, TransitionMap *result){
	bool ret=false;
	for(size_t i=0;i<sizeof(mPossibleChars)/sizeof(bool);++i){
		bool tmp=mPossibleChars[i] && other->mPossibleChars[i];
		result->mPossibleChars[i]=tmp;
		if (tmp) ret=true;
	}
	return ret;
}

void TransitionMap::merge(const TransitionMap* other){
	for(size_t i=0;i<sizeof(mPossibleChars)/sizeof(bool);++i){
		if (other->mPossibleChars[i]) mPossibleChars[i]=true;
	}
}



Recognizer::Recognizer() : mId(0) {
}

void Recognizer::setName(const std::string& name){
	static unsigned int id_base=0;
	mName=name;
	/* every recognizer that is given a name is given also a unique id*/
	mId=++id_base;
}

const string &Recognizer::getName()const{
	return mName;
}

size_t Recognizer::feed(const shared_ptr<ParserContextBase> &ctx, const string &input, size_t pos){
	size_t match;

	ParserLocalContext hctx;
	if (ctx) ctx->beginParse(hctx, shared_from_this());
	match=_feed(ctx, input, pos);
	if (match!=string::npos && match>0){
		if (0 && mName.size()>0){
			string matched=input.substr(pos,match);
			cout<<"Matched recognizer '"<<mName<<"' with sequence '"<<matched<<"'."<<endl;
		}
	}
	if (ctx) ctx->endParse(hctx, input, pos, match);

	return match;
}

bool Recognizer::getTransitionMap(TransitionMap* mask){
	bool ret=_getTransitionMap(mask);
	if (0 /*!mName.empty()*/){
		cout<<"TransitionMap after "<<mName<<endl;
		for(int i=0;i<256;++i){
			if (mask->mPossibleChars[i]) cout<<(char)i;
		}
		cout<<endl;
	}
	return ret;
}


bool Recognizer::_getTransitionMap(TransitionMap* mask){
	string input;
	input.resize(2,'\0');
	for(int i=0;i<256;++i){
		input[0]=i;
		if (feed(NULL,input,0)==1)
			mask->mPossibleChars[i]=true;
	}
	return true;
}

void Recognizer::optimize(){
	optimize(0);
}

void Recognizer::optimize(int recursionLevel){
	if (recursionLevel!=0 && mId!=0) return; /*stop on rule except at level 0*/
	_optimize(++recursionLevel);
}


CharRecognizer::CharRecognizer(int to_recognize, bool caseSensitive) : mToRecognize(to_recognize), mCaseSensitive(caseSensitive){
	if (::tolower(to_recognize)==::toupper(to_recognize)){
		/*not a case caseSensitive character*/
		mCaseSensitive=true;/*no need to perform a case insensitive match*/
	}else if (!caseSensitive){
		mToRecognize=::tolower(mToRecognize);
	}
}

size_t CharRecognizer::_feed(const shared_ptr<ParserContextBase> &ctx, const string &input, size_t pos){
	int c = (unsigned char)input[pos];
	if (mCaseSensitive){
		return c == mToRecognize ? 1 : string::npos;
	}
	return ::tolower(c) == mToRecognize ? 1 : string::npos;
}

void CharRecognizer::_optimize(int recursionLevel){

}

/**
    Saves Char regognizer attributes in a file
		write format is : TAG mCaseSensitive mToRecognize
		TAG is "CR"
    @param std:: ofstream out stream of the file.
    @return void
*/
void CharRecognizer::save(std::ofstream& outFile, long &savePos){
	outFile.seekp(savePos, ios::beg);

		if(outFile){
			outFile << "CR" << " " << mCaseSensitive  << " " <<	mToRecognize << " ";
			savePos = outFile.tellp();
		}
		else{
			cerr << "ERROR writing to file : output stream unavailable" << endl;
		}
}

/**
    Saves Char regognizer attributes in a file
		write format is : TAG SPACE mCaseSensitive : mToRecognize |
		TAG is "CR"
    @param std:: ofstream out stream of the file.
    @return void
*/
void CharRecognizer::saveString(std::ofstream& outFile, long &savePos){
	outFile.seekp(savePos, ios::beg);

		if(outFile){
			outFile << "CR" << " " << mCaseSensitive  << ":" <<	mToRecognize << "|";
			savePos = outFile.tellp();
		}
		else{
			cerr << "ERROR writing to file : output stream unavailable" << endl;
		}
}

/**
    Loads Char regognizer attributes from a file

    @param std::string String containing the recognizer's description.
    @return shared_ptr<CharRecognizer>
*/
shared_ptr<CharRecognizer> CharRecognizer::loadString(std::string& inLine){

	if(!inLine.empty()){

		typedef boost::tokenizer<boost::char_separator<char>>tokenizer;
		boost::char_separator<char> sep(" ");

		tokenizer tokens(inLine, sep);
//		for (tokenizer::iterator tok_iter = tokens.begin();tok_iter != tokens.end(); ++tok_iter)
//			std::cout <<"DEBUG: parsed element"<<  "<" << *tok_iter << "> ";
		tokenizer::iterator tok_iter = tokens.begin();
			cout << endl;
		if (*(tok_iter) == "CR"){
			string temp = *(++tok_iter);
			bool mCaseSensitive_loaded = stoi(temp);
		 	temp = *(++tok_iter);
			int mToRecognize_loaded = stoi(temp);



			return make_shared<CharRecognizer>(mToRecognize_loaded , mCaseSensitive_loaded);
		}
		else
			cerr << "ERROR reading file : no CHAR RECOGNIZER tag found" << endl;

	}
	else
		cerr << "ERROR reading file : lecture du String impossible" << endl;
		return NULL;

}

/**
    Loads Char regognizer attributes from a file

    @param std::ifstream out stream of the file.
    @return shared_ptr<CharRecognizer>
*/
shared_ptr<CharRecognizer> CharRecognizer::load(std::ifstream& inFile, long &loadPos){
	inFile.seekg(loadPos, ios::beg);

	if(inFile){
		bool mCaseSensitive_loaded = false;
		int mToRecognize_loaded = 0;
		std::string tag = "";
		inFile >> tag ;

			if (tag == "CR"){
				inFile >> mCaseSensitive_loaded >> mToRecognize_loaded;
				loadPos = inFile.tellg();
			}
			else{
				cerr << "ERROR reading file : no CHAR RECOGNIZER tag found" << endl;
			}
			return make_shared<CharRecognizer>(mToRecognize_loaded, mCaseSensitive_loaded);
		}
	else{
		cerr << "ERROR reading file : lecture du fichier impossible" << endl;
	}
	return NULL;
}

/**
    Loads Char regognizer attributes from a file

    @param std::string out stream of the file.
    @return shared_ptr<CharRecognizer>
*/
shared_ptr<CharRecognizer> CharRecognizer::loadStringed(std::string& inLine, long &loadPos){

	if(!inLine.empty()){

		std::string tag = parseStr(inLine, loadPos); //parses the first element of the space separated string starting from strlPos
//		cout << "DEBUG : tag CR :" << tag << " | ";
			if (tag == "CR"){
				string temp = parseStr(inLine, loadPos);
		//		cout << "content : <" << temp <<">,<";
				bool mCaseSensitive_loaded  = stoi(temp);
				temp = parseStr(inLine, loadPos);
	//			cout << temp << ">"<< endl;
				int mToRecognize_loaded = stoi(temp);
				return make_shared<CharRecognizer>(mToRecognize_loaded, mCaseSensitive_loaded);
			}
			else{
				cerr << "ERROR no CHAR RECOGNIZER tag found" << endl;
			}
		}
	else{
		cerr << "ERROR loaded string is empty" << endl;
	}
	return NULL;
}


shared_ptr<CharRecognizer> CharRecognizer::loadVect(std::vector<string>::const_iterator &inIter){
	//cout << "DEBUG : tag : <" << *(inIter) << ">";
	if (*(inIter) == "CR"){
		string temp = *(++inIter);
	//	cout << " | caseSens : <" << *(inIter) << "> | ";

		bool mCaseSensitive_loaded = stoi(temp);
	 	temp = *(++inIter);
	//	cout << "Caracter : <" << *(inIter) << ">"<< endl;

		int mToRecognize_loaded = stoi(temp);
		++inIter;
		return make_shared<CharRecognizer>(mToRecognize_loaded , mCaseSensitive_loaded);
	}
	else
		cerr << "ERROR reading file : no CHAR RECOGNIZER tag found" << endl;
	return NULL;
}







bool CharRecognizer::equal(const shared_ptr<Recognizer> &CR){
	//Using dynamic cast to have acces to the attributes of the CharRecognizer object
	const shared_ptr<CharRecognizer> CR_local = dynamic_pointer_cast<CharRecognizer>(CR);
	return((CR_local->mToRecognize == mToRecognize) && (CR_local->mCaseSensitive == mCaseSensitive)) ? true : false;
}

//DEBUG FUNCTION
void CharRecognizer::printtype(){
	cout << " type:CR ";
}

Selector::Selector() : mIsExclusive(false){
}

shared_ptr<Selector> Selector::addRecognizer(const shared_ptr<Recognizer> &r){
	mElements.push_back(r);
	return static_pointer_cast<Selector> (shared_from_this());
}

bool Selector::_getTransitionMap(TransitionMap* mask){
    for (auto it=mElements.begin(); it!=mElements.end(); ++it){
	    (*it)->getTransitionMap(mask);
    }
    return true;
}

size_t Selector::_feedExclusive(const shared_ptr<ParserContextBase> &ctx, const string &input, size_t pos){
	size_t matched=0;

	for (auto it=mElements.begin(); it!=mElements.end(); ++it){
		matched=(*it)->feed(ctx, input, pos);
		if (matched!=string::npos && matched>0) {
			return matched;
		}
	}
	return string::npos;
}

size_t Selector::_feed(const shared_ptr<ParserContextBase> &ctx, const string &input, size_t pos){
	if (mIsExclusive) return _feedExclusive(ctx, input, pos);

	size_t matched=0;
	size_t bestmatch=0;
	shared_ptr<HandlerContextBase> bestBranch;

	for (auto it=mElements.begin(); it!=mElements.end(); ++it){
		shared_ptr<HandlerContextBase> br;
		if (ctx) br=ctx->branch();
		matched=(*it)->feed(ctx, input, pos);
		if (matched!=string::npos && matched>bestmatch) {
			bestmatch=matched;
			if (bestBranch) ctx->removeBranch(bestBranch);
			bestBranch=br;
		}else{
			if (ctx)
				ctx->removeBranch(br);
		}
	}
	if (bestmatch==0) return string::npos;
	if (ctx && bestmatch!=string::npos){
		ctx->merge(bestBranch);
	}
	return bestmatch;
}

void Selector::_optimize(int recursionLevel){
	for (auto it=mElements.begin(); it!=mElements.end(); ++it){
		(*it)->optimize(recursionLevel);
	}
	TransitionMap *all=NULL;
	bool intersectionFound=false;
	for (auto it=mElements.begin(); it!=mElements.end() && !intersectionFound; ++it){
		TransitionMap *cur=new TransitionMap();
		(*it)->getTransitionMap(cur);
		if (all){
			if (cur->intersect(all)){
				intersectionFound=true;
			}
			all->merge(cur);
			delete cur;
		}else all=cur;
	}
	if (all) delete all;
	if (!intersectionFound){
		//cout<<"Selector '"<<getName()<<"' is exclusive."<<endl;
		mIsExclusive=true;
	}
}
/**
    Saves Selector attributes in a file
		write format is : TAG mIsExclusive numberOfElements :: Element1 Element2 Element3
		each Element is a recognizer meaning it has got its own format
		TAG is "SEL"
    @param std:: ofstream out stream of the file.
    @return void
*/
void Selector::saveString(std::ofstream& outFile, long &savePos){
	outFile.seekp(savePos, ios::beg);

		if(outFile){
			outFile << "SEL" << " " << mIsExclusive  << " ";
			savePos = outFile.tellp();
			int recognizerListSize = 0;

			//the number of elements in the recognizer list is needed when loading the Selector afterwards
			for(list<shared_ptr<Recognizer>>::iterator it = mElements.begin(); it!=mElements.end(); ++it){
				recognizerListSize++ ;
			}


			outFile << recognizerListSize << ":";

			for(list<shared_ptr<Recognizer>>::iterator it = mElements.begin(); it!=mElements.end() ; ++it){
						(*it)->save(outFile, savePos);

 			}
		}
		else{
			cerr << "ecriture dans le fichier impossibe" << endl;
		}
}


/**
    Saves Selector attributes in a file
		write format is : TAG mIsExclusive numberOfElements Element1 Element2 Element3
		each Element is a recognizer meaning it has got its own format
		TAG is "SEL"
    @param std:: ofstream out stream of the file.
    @return void
*/
void Selector::save(std::ofstream& outFile, long &savePos){
	outFile.seekp(savePos, ios::beg);

		if(outFile){
			outFile << "SEL" << " " << mIsExclusive  << " ";
			savePos = outFile.tellp();
			int recognizerListSize = 0;

			//the number of elements in the recognizer list is needed when loading the Selector afterwards
			for(list<shared_ptr<Recognizer>>::iterator it = mElements.begin(); it!=mElements.end(); ++it){
				recognizerListSize++ ;
			}


			outFile << recognizerListSize << " ";

			for(list<shared_ptr<Recognizer>>::iterator it = mElements.begin(); it!=mElements.end() ; ++it){
						(*it)->save(outFile, savePos);

 			}
		}
		else{
			cerr << "ecriture dans le fichier impossibe" << endl;
		}
}

shared_ptr<Selector> Selector::loadVect(std::vector<string>::const_iterator &inIter){
//	cout << "DEBUG : tag : <" << *(inIter) << ">";

	shared_ptr<Selector> selector_reloaded = NULL;

	if(*inIter == "SEL"){
		list<shared_ptr<Recognizer>> mElements_loaded;
		bool mIsExclusive_loaded = stoi(*(++inIter));
//		cout << " | IsEx : <" << *(inIter) << "> | ";
		int recognizerListSize = stoi(*(++inIter));
//		cout << "NbElem : <" << *(inIter) << ">"<< endl;
		selector_reloaded = Foundation::selector(mIsExclusive_loaded);
		int i=0;
		++inIter;

		for(list<shared_ptr<Recognizer>>::const_iterator it = mElements_loaded.begin(); i<recognizerListSize; ++it){
      if (*(inIter) == "CR"){
				selector_reloaded->addRecognizer(CharRecognizer::loadVect(inIter));
			}
      else if(*(inIter) == "SEQ"){
      	selector_reloaded->addRecognizer(Sequence::loadVect(inIter));
      }
      else if(*(inIter) == "LOOP"){
        selector_reloaded->addRecognizer(Loop::loadVect(inIter));
      }
      else if(*(inIter) == "XCR"){
        selector_reloaded->addRecognizer(CharRange::loadVect(inIter));
      }
      else if(*(inIter) == "LIT"){
        selector_reloaded->addRecognizer(Literal::loadVect(inIter));
      }
      else if(*(inIter) == "XSEL"){
      	selector_reloaded->addRecognizer(ExclusiveSelector::loadVect(inIter));
      }
      else if(*(inIter) == "SEL"){
      	selector_reloaded->addRecognizer(Selector::loadVect(inIter));
      }
      else if(*(inIter) == "RECP"){
        selector_reloaded->addRecognizer(RecognizerPointer::loadVect(inIter));
      }
      else
							std::cerr << "ERROR : inside selector Type not recognized" << '\n';
			i++;
				}
			}
			else
				cerr << "ERROR : No SELECTOR tag found" << endl;

	return selector_reloaded;

}
/**
    Loads Selector attributes from a file

    @param std::ifstream out stream of the file.
    @return shared_ptr<Selector>
*/
shared_ptr<Selector> Selector::load(std::ifstream& inFile, long &loadPos){

	inFile.seekg(loadPos, ios::beg);

	shared_ptr<Selector> selector_reloaded = NULL;
	shared_ptr<Recognizer> temp;

	if(inFile){

		bool mIsExclusive_loaded ;
		list<shared_ptr<Recognizer>> mElements_loaded;

		int recognizerListSize =0;
		std::string tag = "";
		inFile >> tag ;

			if (tag == "SEL"){
				inFile >> mIsExclusive_loaded;
				inFile >> recognizerListSize;
				loadPos = inFile.tellg();

				selector_reloaded = Foundation::selector(mIsExclusive_loaded);

				int i = 0;
				for(list<shared_ptr<Recognizer>>::const_iterator it = mElements_loaded.begin(); i<recognizerListSize; ++it){

					inFile.seekg(loadPos, ios::beg);

					long pos_pre_tag = inFile.tellg();
					inFile >> tag;

					if (tag == "CR"){
							inFile.seekg(pos_pre_tag, ios::beg);
							selector_reloaded->addRecognizer(CharRecognizer::load(inFile, loadPos));
					}
					else if(tag == "SEQ"){
						inFile.seekg(pos_pre_tag, ios::beg);
						selector_reloaded->addRecognizer(Sequence::load(inFile, loadPos));
					}
					else if(tag == "LOOP"){
						inFile.seekg(pos_pre_tag, ios::beg);
						selector_reloaded->addRecognizer(Loop::load(inFile, loadPos));
					}
					else if(tag == "XCR"){
						inFile.seekg(pos_pre_tag, ios::beg);
						selector_reloaded->addRecognizer(CharRange::load(inFile, loadPos));
					}
					else if(tag == "LIT"){
						inFile.seekg(pos_pre_tag, ios::beg);
						selector_reloaded->addRecognizer(Literal::load(inFile, loadPos));
					}
					else if(tag == "XSEL"){
						inFile.seekg(pos_pre_tag, ios::beg);
						selector_reloaded->addRecognizer(ExclusiveSelector::load(inFile, loadPos));
					}
					else if(tag == "SEL"){
						inFile.seekg(pos_pre_tag, ios::beg);
						selector_reloaded->addRecognizer(Selector::load(inFile, loadPos));
					}
					else if(tag == "RECP"){
						inFile.seekg(pos_pre_tag, ios::beg);
						selector_reloaded->addRecognizer(RecognizerPointer::load(inFile, loadPos));
					}
					else
							std::cerr << "ERROR : Type not recognized" << '\n';
					i++;

				}

			}
			else{
				cerr << "ERROR : No SELECTOR tag found" << endl;
			}
	}
	else{
		cerr << "ERROR : file stream unavailable" << endl;
	}
	return selector_reloaded;
}


bool Selector::equal(const std::shared_ptr<Recognizer> &SEL){

	const shared_ptr<Selector> SEL_local = dynamic_pointer_cast<Selector>(SEL);

		list<shared_ptr<Recognizer>>::iterator i_local = SEL_local->mElements.begin();

		list<shared_ptr<Recognizer>>::iterator i = mElements.begin();

	bool condition = true;
	while (condition == true && i!=mElements.end()){
		if (!(*i)->equal(*i_local)) {
			condition = false;
		}
		advance(i,1);
		advance(i_local,1);

	}
	return(condition && (SEL_local->mIsExclusive == mIsExclusive)) ? true : false;

}

//DEBUG FUNCTION
void Selector::printtype(){
	cout << "type : SEL";
	for(list<shared_ptr<Recognizer>>::iterator it = mElements.begin(); it!=mElements.end() ; ++it){
				(*it)->printtype();
	}
}

ExclusiveSelector::ExclusiveSelector() {
	mIsExclusive = true;
}

size_t ExclusiveSelector::_feed(const shared_ptr<ParserContextBase> &ctx, const string &input, size_t pos){
	return Selector::_feedExclusive(ctx, input, pos);
}

void ExclusiveSelector::save(std::ofstream& outFile, long &savePos){
		outFile.seekp(savePos, ios::beg);
		if(outFile){
			outFile << "XSEL" << " " << mIsExclusive  << " ";

			int exRecognizerListSize = 0;

			for(list<shared_ptr<Recognizer>>::iterator it = mElements.begin(); it!=mElements.end(); ++it){
				exRecognizerListSize++ ;
			}

			outFile << exRecognizerListSize << " ";
			int i =0;
			savePos = outFile.tellp(); // récuppération de la dernière position d'écriture et affectation dans savePos

			for(list<shared_ptr<Recognizer>>::iterator it = mElements.begin(); i<exRecognizerListSize ; ++it){
					(*it)->save(outFile, savePos);
					i++;
 			}
		}
		else{
			cerr << "ERROR : file stream unavailable" << endl;
		}
}


shared_ptr<ExclusiveSelector> ExclusiveSelector::load(std::ifstream& inFile, long &loadPos){

	inFile.seekg(loadPos, ios::beg);

	shared_ptr<Selector> selector_reloaded = NULL;

	if(inFile){

		bool mIsExclusive_loaded ;
		list<shared_ptr<Recognizer>> mElements_loaded;

		int exRecognizerListSize =0;
		std::string tag = "";
		inFile >> tag ;
			if (tag == "XSEL"){
				inFile >> mIsExclusive_loaded;
				inFile >> exRecognizerListSize;
				loadPos = inFile.tellg();
				selector_reloaded = Foundation::selector(mIsExclusive_loaded);
				int i = 0;
				for(list<shared_ptr<Recognizer>>::const_iterator it = mElements_loaded.begin(); i<exRecognizerListSize; ++it){ // TODO AGAIN
					inFile >> tag;
					if (tag == "CR"){
							selector_reloaded->addRecognizer(CharRecognizer::load(inFile, loadPos));
					}
					else if(tag == "SEQ"){
						selector_reloaded->addRecognizer(Sequence::load(inFile, loadPos));
					}
					else if(tag == "LOOP"){
						selector_reloaded->addRecognizer(Loop::load(inFile, loadPos));
					}
					else if(tag == "XCR"){
						selector_reloaded->addRecognizer(CharRange::load(inFile, loadPos));
					}
					else if(tag == "LIT"){
						selector_reloaded->addRecognizer(Literal::load(inFile, loadPos));
					}
					else if(tag == "SEL"){
						selector_reloaded->addRecognizer(Selector::load(inFile, loadPos));
					}
					else if(tag == "RECP"){
						selector_reloaded->addRecognizer(RecognizerPointer::load(inFile, loadPos));
					}
					else if(tag == "XSEL"){
						selector_reloaded->addRecognizer(ExclusiveSelector::load(inFile, loadPos));
					}
					else
							std::cerr << "ERROR : Type inside exclusive selector not recognized" << '\n';
					i++;

				}

			}
			else{
				cerr << "ERROR : no EXCLUSIVESELECTOR tag found" << endl;
			}
	}
	else{
		cerr << "ERROR : file stream unavailable" << endl;
	}
	return dynamic_pointer_cast<ExclusiveSelector>(selector_reloaded);
}

shared_ptr<ExclusiveSelector> ExclusiveSelector::loadVect(std::vector<string>::const_iterator &inIter){
//	cout << "DEBUG : tag : <" << *(inIter) << ">";

	shared_ptr<Selector> selector_reloaded = NULL;

	if(*inIter == "XSEL"){
		list<shared_ptr<Recognizer>> mElements_loaded;
		bool mIsExclusive_loaded = stoi(*(++inIter));
//		cout << " | IsEx : <" << *(inIter) << "> | ";
		int recognizerListSize = stoi(*(++inIter));
	//	cout << "NbElem : <" << *(inIter) << ">"<< endl;
		selector_reloaded = Foundation::selector(mIsExclusive_loaded);
		int i=0;
		++inIter;
		for(list<shared_ptr<Recognizer>>::const_iterator it = mElements_loaded.begin(); i<recognizerListSize; ++it){
	//		cout << "DEBUG : tag inside : <" << *inIter << ">" << endl;
			if (*(inIter) == "CR"){
				selector_reloaded->addRecognizer(CharRecognizer::loadVect(inIter));
			}
			else if(*(inIter) == "XSEL"){
				selector_reloaded->addRecognizer(ExclusiveSelector::loadVect(inIter));
			}
			else if(*(inIter) == "SEL"){
				selector_reloaded->addRecognizer(Selector::loadVect(inIter));
			}
			else if(*(inIter) == "SEQ"){
      	selector_reloaded->addRecognizer(Sequence::loadVect(inIter));
      }
      else if(*(inIter) == "LOOP"){
        selector_reloaded->addRecognizer(Loop::loadVect(inIter));
      }
      else if(*(inIter) == "XCR"){
        selector_reloaded->addRecognizer(CharRange::loadVect(inIter));
      }
      else if(*(inIter) == "LIT"){
        selector_reloaded->addRecognizer(Literal::loadVect(inIter));
      }
      else if(*(inIter) == "RECP"){
      	selector_reloaded->addRecognizer(RecognizerPointer::loadVect(inIter));
      }
      else
							std::cerr << "ERROR : Type not recognized" << '\n';
			i++;
				}
			}
			else
				cerr << "ERROR : No SELECTOR tag found" << endl;

			return dynamic_pointer_cast<ExclusiveSelector>(selector_reloaded);
}


bool ExclusiveSelector::equal(const std::shared_ptr<Recognizer> &SEL){

	const shared_ptr<ExclusiveSelector> SEL_local = dynamic_pointer_cast<ExclusiveSelector>(SEL);
	list<shared_ptr<Recognizer>>::iterator i_local = SEL_local->mElements.begin();
	list<shared_ptr<Recognizer>>::iterator i = mElements.begin();
	bool condition = true;

	while (condition == true && i!=mElements.end()){
		if (!(*i)->equal(*i_local)) condition = false;
		advance(i,1);
		advance(i_local,1);
	}
	return(condition && (SEL_local->mIsExclusive == mIsExclusive)) ? true : false;
}

//DEBUG FUNCTION
void ExclusiveSelector::printtype(){
	cout << "type : XSEL";
	for(list<shared_ptr<Recognizer>>::iterator it = mElements.begin(); it!=mElements.end() ; ++it){
				(*it)->printtype();
	}
}


Sequence::Sequence(){
}

shared_ptr<Sequence> Sequence::addRecognizer(const shared_ptr<Recognizer> &element){
	mElements.push_back(element);
	return static_pointer_cast<Sequence>( shared_from_this());
}

bool Sequence::_getTransitionMap(TransitionMap* mask){
	bool isComplete=false;
	for (auto it=mElements.begin(); it!=mElements.end(); ++it){
		if ((*it)->getTransitionMap(mask)) {
			isComplete=true;
			break;
		}
	}
	return isComplete;
}


size_t Sequence::_feed(const shared_ptr<ParserContextBase> &ctx, const string &input, size_t pos){
	size_t matched=0;
	size_t total=0;

	for (auto it=mElements.begin(); it!=mElements.end(); ++it){
		matched=(*it)->feed(ctx, input, pos);
		if (matched==string::npos){
			return string::npos;
		}
		pos+=matched;
		total+=matched;
	}
	return total;
}

void Sequence::_optimize(int recursionLevel){
	for (auto it=mElements.begin(); it!=mElements.end(); ++it)
		(*it)->optimize(recursionLevel);
}

void Sequence::save(std::ofstream& outFile, long &savePos){

		outFile.seekp(savePos, ios::beg);
		if(outFile){
			outFile << "SEQ" << " ";

			int sequenceListSize = 0;

			for(list<shared_ptr<Recognizer>>::iterator it = mElements.begin(); it!=mElements.end(); ++it){
				sequenceListSize++ ;
			}

			outFile << sequenceListSize << " ";
			int i =0;
			savePos = outFile.tellp(); // récuppération de la dernière position d'écriture et affectation dans savePos

			for(list<shared_ptr<Recognizer>>::iterator it = mElements.begin(); i<sequenceListSize ; ++it){
				shared_ptr<Recognizer> re = (shared_ptr<Recognizer>)*it;
				re->save(outFile, savePos);
				i++;
 			}
		}
		else{
			cerr << "ERROR : file stream unavailable" << endl;
		}
}

shared_ptr<Sequence> Sequence::load(std::ifstream& inFile, long &loadPos){
	inFile.seekg(loadPos, ios::beg);

	shared_ptr<Sequence> sequence_reloaded = NULL; //type de retour
	shared_ptr<Recognizer> temp;

	if(inFile){
		list<shared_ptr<Recognizer>> mElements_loaded;

		int sequenceListSize =0;
		std::string tag = "";
		inFile >> tag ;

			if (tag == "SEQ"){
				inFile >> sequenceListSize;
				loadPos = inFile.tellg();
				sequence_reloaded = make_shared<Sequence>();

				int i = 0;
				for(list<shared_ptr<Recognizer>>::const_iterator it = mElements_loaded.begin(); i<sequenceListSize; ++it){ // TODO AGAIN

					inFile >> tag;
					if (tag == "CR"){
							sequence_reloaded->addRecognizer(CharRecognizer::load(inFile, loadPos));
					}
					else if(tag == "SEL"){
						sequence_reloaded->addRecognizer(Selector::load(inFile, loadPos));
					}
					else if(tag == "XSEL"){
						sequence_reloaded->addRecognizer(ExclusiveSelector::load(inFile, loadPos));
					}
					else if(tag == "XCR"){
						sequence_reloaded->addRecognizer(CharRange::load(inFile, loadPos));
					}
					else if(tag == "LIT"){
						sequence_reloaded->addRecognizer(Literal::load(inFile, loadPos));
					}
					else if(tag == "LOOP"){
						sequence_reloaded->addRecognizer(Loop::load(inFile, loadPos));
					}
					else if(tag == "RECP"){
						sequence_reloaded->addRecognizer(RecognizerPointer::load(inFile, loadPos));
					}
					else if(tag == "SEQ"){
						sequence_reloaded->addRecognizer(Sequence::load(inFile, loadPos));
					}
					else
							std::cerr << "ERROR : TAG not recognized" << '\n';
					i++;
				}
			}
			else{
				cerr << "ERROR : no SEQUENCE tag found" << endl;
			}
	}
	else{
		cerr << "ERROR : file stream unavailable" << endl;
	}
	return sequence_reloaded;
}

shared_ptr<Sequence> Sequence::loadVect(std::vector<string>::const_iterator &inIter){
	//cout << "DEBUG : tag : <" << *(inIter) << ">";

	shared_ptr<Sequence> sequence_reloaded = NULL; //type de retour

	if(*inIter == "SEQ"){
		list<shared_ptr<Recognizer>> mElements_loaded;
		int recognizerListSize = stoi(*(++inIter));
	//	cout << " | NbElem : <" << *(inIter) << ">"<< endl;
		sequence_reloaded = make_shared<Sequence>();
		int i=0;
		++inIter;
		for(list<shared_ptr<Recognizer>>::const_iterator it = mElements_loaded.begin(); i<recognizerListSize; ++it){
			if (*(inIter) == "CR"){
				sequence_reloaded->addRecognizer(CharRecognizer::loadVect(inIter));
			}
			else if(*(inIter) == "XSEL"){
				sequence_reloaded->addRecognizer(ExclusiveSelector::loadVect(inIter));
			}
			else if(*(inIter) == "SEL"){
				sequence_reloaded->addRecognizer(Selector::loadVect(inIter));
			}
			else if(*(inIter) == "SEQ"){
      	sequence_reloaded->addRecognizer(Sequence::loadVect(inIter));
      }
      else if(*(inIter) == "LOOP"){
        sequence_reloaded->addRecognizer(Loop::loadVect(inIter));
      }
      else if(*(inIter) == "XCR"){
        sequence_reloaded->addRecognizer(CharRange::loadVect(inIter));
      }
      else if(*(inIter) == "LIT"){
        sequence_reloaded->addRecognizer(Literal::loadVect(inIter));
      }
      else if(*(inIter) == "RECP"){
      	sequence_reloaded->addRecognizer(RecognizerPointer::loadVect(inIter));
      }
      else
							std::cerr << "ERROR : Type not recognized" << '\n';
			i++;
				}
			}
			else
				cerr << "ERROR : No SELECTOR tag found" << endl;

			return (sequence_reloaded);
}



bool Sequence::equal(const std::shared_ptr<Recognizer> &SEQ){

	const shared_ptr<Sequence> SEQ_local = dynamic_pointer_cast<Sequence>(SEQ);
	list<shared_ptr<Recognizer>>::iterator i_local = SEQ_local->mElements.begin();
	list<shared_ptr<Recognizer>>::iterator i = mElements.begin();
	bool condition = true;

	while (condition == true && i!=mElements.end()){
		if (!(*i)->equal(*i_local)) {
			condition = false;
		}
		advance(i,1);
		advance(i_local,1);
	}
	return(condition) ? true : false;
}

//DEBUG FUNCTION
void Sequence::printtype(){
	cout << "type : SEQ";
	for(list<shared_ptr<Recognizer>>::iterator it = mElements.begin(); it!=mElements.end() ; ++it){
				(*it)->printtype();
	}
}


Loop::Loop() : mMin(0), mMax(-1) {

}

shared_ptr<Loop> Loop::setRecognizer(const shared_ptr<Recognizer> &element, int min, int max){
	mMin=min;
	mMax=max;
	mRecognizer=element;
	return static_pointer_cast<Loop>(shared_from_this());
}

size_t Loop::_feed(const shared_ptr<ParserContextBase> &ctx, const string &input, size_t pos){
	size_t matched=0;
	size_t total=0;
	int repeat;

	for(repeat=0;mMax!=-1 ? repeat<mMax : true;repeat++){
		matched=mRecognizer->feed(ctx, input, pos);
		if (matched==string::npos) break;
		total+=matched;
		pos+=matched;
		if (input[pos]=='\0') break;
	}
	if (repeat<mMin) return string::npos;
	return total;
}

bool Loop::_getTransitionMap(TransitionMap* mask){
	mRecognizer->getTransitionMap(mask);
	return mMin!=0; //we must say to upper layer that this loop recognizer is allowed to be optional by returning FALSE
}

void Loop::_optimize(int recursionLevel){
	mRecognizer->optimize(recursionLevel);
}

//MODIFIE PAR IYED
void Loop::save(std::ofstream& outFile, long &savePos){

		outFile.seekp(savePos, ios::beg);
		if(outFile){
			outFile << "LOOP" << " " << mMin << " " << mMax << " ";
			mRecognizer->save(outFile, savePos);
		}
		else{
			cerr << "ecriture dans le fichier impossibe" << endl;
		}
}

shared_ptr<Loop> Loop::load(std::ifstream& inFile, long &loadPos){
	inFile.seekg(loadPos, ios::beg);

	shared_ptr<Loop> loop_reloaded = NULL; //type de retour

	if(inFile){

		int mMin_loaded;
		int mMax_loaded;


		std::string tag = "";
		inFile >> tag ;


			if (tag == "LOOP"){

				inFile >> mMin_loaded >> mMax_loaded;
				loadPos = inFile.tellg();
				//	long pos_pre_tag = inFile.tellg();
					inFile >> tag;

					if (tag == "CR"){

				//			inFile.seekg(pos_pre_tag, ios::beg);
							const shared_ptr<CharRecognizer> mRecognizer_loaded = CharRecognizer::load(inFile, loadPos);
							loop_reloaded = make_shared<Loop>();
							loop_reloaded->setRecognizer(mRecognizer_loaded, mMin_loaded, mMax_loaded); //houni errue

					}
					else if(tag == "SEL"){

			//			inFile.seekg(pos_pre_tag, ios::beg);
						const shared_ptr<Selector> mRecognizer_loaded = (Selector::load(inFile, loadPos));
						loop_reloaded = make_shared<Loop>();
						loop_reloaded->setRecognizer(mRecognizer_loaded, mMin_loaded, mMax_loaded);
					}
					else if(tag == "XSEL"){
				//		inFile.seekg(pos_pre_tag, ios::beg);

						const shared_ptr<ExclusiveSelector> mRecognizer_loaded = (ExclusiveSelector::load(inFile, loadPos));
						loop_reloaded = make_shared<Loop>();
						loop_reloaded->setRecognizer(mRecognizer_loaded, mMin_loaded, mMax_loaded);
					}
					else if(tag == "SEQ"){
					//	inFile.seekg(pos_pre_tag, ios::beg);
						const shared_ptr<Sequence> mRecognizer_loaded = (Sequence::load(inFile, loadPos));
						loop_reloaded = make_shared<Loop>();
						loop_reloaded->setRecognizer(mRecognizer_loaded, mMin_loaded, mMax_loaded);
					}
					else if(tag == "XCR"){
				//		inFile.seekg(pos_pre_tag, ios::beg);
						const shared_ptr<CharRange> mRecognizer_loaded = (CharRange::load(inFile, loadPos));
						loop_reloaded = make_shared<Loop>();
						loop_reloaded->setRecognizer(mRecognizer_loaded, mMin_loaded, mMax_loaded);
					}
					else if(tag == "LIT"){
			//			inFile.seekg(pos_pre_tag, ios::beg);
						const shared_ptr<Literal> mRecognizer_loaded = (Literal::load(inFile, loadPos));
						loop_reloaded = make_shared<Loop>();
						loop_reloaded->setRecognizer(mRecognizer_loaded, mMin_loaded, mMax_loaded);
					}
					else if(tag == "RECP"){
			//			inFile.seekg(pos_pre_tag, ios::beg);
						const shared_ptr<RecognizerPointer> mRecognizer_loaded = (RecognizerPointer::load(inFile, loadPos));
						loop_reloaded = make_shared<Loop>();
						loop_reloaded->setRecognizer(mRecognizer_loaded, mMin_loaded, mMax_loaded);
					}
					else if(tag == "LOOP"){
			//			inFile.seekg(pos_pre_tag, ios::beg);
						const shared_ptr<Loop> mRecognizer_loaded = (Loop::load(inFile, loadPos));
						loop_reloaded = make_shared<Loop>();
						loop_reloaded->setRecognizer(mRecognizer_loaded, mMin_loaded, mMax_loaded);
					}
					else
							std::cerr << "Recognizer type undefined" << '\n';



			}
			else{
				cerr << "no LOOP tag found" << endl;
			}
	}
	else{
		cerr << "lecture du fichier impossible" << endl;
	}

	return loop_reloaded;
}

shared_ptr<Loop> Loop::loadVect(std::vector<string>::const_iterator &inIter){
//	cout << "DEBUG : tag : <" << *(inIter) << ">";

	shared_ptr<Loop> loop_reloaded = NULL; //type de retour

	if(*inIter == "LOOP"){
//		list<shared_ptr<Recognizer>> mElements_loaded;
    int mMin_loaded =  stoi(*(++inIter));
//    cout << " | mMin : <" << *(inIter) << ">"<< endl;

    int mMax_loaded =  stoi(*(++inIter));

	//	cout << " | mMax : <" << *(inIter) << ">"<< endl;
    loop_reloaded = make_shared<Loop>();
		++inIter;
			if (*(inIter) == "CR"){
        loop_reloaded->setRecognizer(CharRecognizer::loadVect(inIter), mMin_loaded, mMax_loaded); //houni errue
			}
			else if(*(inIter) == "XSEL"){
        loop_reloaded->setRecognizer(ExclusiveSelector::loadVect(inIter), mMin_loaded, mMax_loaded); //houni errue
			}
			else if(*(inIter) == "SEL"){
        loop_reloaded->setRecognizer(Selector::loadVect(inIter), mMin_loaded, mMax_loaded); //houni errue
			}
			else if(*(inIter) == "SEQ"){
        loop_reloaded->setRecognizer(Sequence::loadVect(inIter), mMin_loaded, mMax_loaded); //houni errue
      }
      else if(*(inIter) == "LOOP"){
        loop_reloaded->setRecognizer(Loop::loadVect(inIter), mMin_loaded, mMax_loaded); //houni errue
      }
      else if(*(inIter) == "XCR"){
        loop_reloaded->setRecognizer(CharRange::loadVect(inIter), mMin_loaded, mMax_loaded); //houni errue
      }
      else if(*(inIter) == "LIT"){
        loop_reloaded->setRecognizer(Literal::loadVect(inIter), mMin_loaded, mMax_loaded); //houni errue
      }
      else if(*(inIter) == "RECP"){
        loop_reloaded->setRecognizer(RecognizerPointer::loadVect(inIter), mMin_loaded, mMax_loaded); //houni errue
      }
      else
				std::cerr << "ERROR : Type not recognized" << '\n';
		}
		else
			cerr << "ERROR : No SELECTOR tag found" << endl;

	return (loop_reloaded);
}



bool Loop::equal(const std::shared_ptr<Recognizer> &LOP){
	const shared_ptr<Loop> LOP_local = dynamic_pointer_cast<Loop>(LOP);

	return((LOP_local->mMin == mMin) && (LOP_local->mMax == mMax) && (mRecognizer->equal(LOP_local->mRecognizer))) ? true : false;
}

//DEBUG FUNCTION
void Loop::printtype(){
	cout << "type : Loop";
	//DEBUG FUNCTION
							mRecognizer->printtype();

}

CharRange::CharRange(int begin, int end) : mBegin(begin), mEnd(end){
}

size_t CharRange::_feed(const shared_ptr<ParserContextBase> &ctx, const string &input, size_t pos){
	int c = (unsigned char)input[pos];
	if (c >= mBegin && c <= mEnd) return 1;
	return string::npos;
}

void CharRange::_optimize(int recursionLevel){

}

void CharRange::save(std::ofstream& outFile, long &savePos){

		outFile.seekp(savePos, ios::beg);

		if(outFile){

			outFile << "XCR" << " " << mBegin  << " " <<	mEnd << " ";
			savePos = outFile.tellp();
		}
		else{
			cerr << "error writing to file : ecriture dans le fichier impossibe" << endl;
		}
}


//MODIFIE PAR IYED
shared_ptr<CharRange> CharRange::load(std::ifstream& inFile, long &loadPos){

	inFile.seekg(loadPos, ios::beg);
	if(inFile){
		int mBegin_loaded = false;
		int mEnd_loaded = 0;
		std::string tag = "";
		inFile >> tag ;

			if (tag == "XCR"){
				inFile >> mBegin_loaded >> mEnd_loaded;

				loadPos = inFile.tellg();
			}
			else{
				cerr << "ERROR : no CHARRANGE tag found" << endl;
			}
			return make_shared<CharRange>(mBegin_loaded, mEnd_loaded);
		}
	else{
		cerr << "ERROR : file stream unavailable" << endl;
	}
	return NULL;
}

shared_ptr<CharRange> CharRange::loadVect(std::vector<string>::const_iterator &inIter){
//  cout << "DEBUG : tag : <" << *(inIter) << ">";

	if(*(inIter) == "XCR"){
    int mBegin_loaded =  stoi(*(++inIter));
//    cout << " | mMin : <" << *(inIter) << ">"<< endl;

    int mEnd_loaded =  stoi(*(++inIter));
  //  cout << " | mMin : <" << *(inIter) << ">"<< endl;
    ++inIter;
    return make_shared<CharRange>(mBegin_loaded, mEnd_loaded);
  }
	else{
		cerr << "ERROR : no CHARRANGE tag found" << endl;
	}
  return NULL;
}



bool CharRange::equal(const shared_ptr<Recognizer> &XCR){
	const shared_ptr<CharRange> XCR_local = dynamic_pointer_cast<CharRange>(XCR); //cast en type approprié pour pouvoir accéder aux attributs

	return((XCR_local->mBegin == mBegin) && (XCR_local->mEnd == mEnd)) ? true : false;
}

//DEBUG FUNCTION
void CharRange::printtype(){
	cout << "type : CharRange";

}


shared_ptr<CharRecognizer> Foundation::charRecognizer(int character, bool caseSensitive){
	return make_shared<CharRecognizer>(character, caseSensitive);
}

shared_ptr<Selector> Foundation::selector(bool isExclusive){
	return isExclusive ? make_shared<ExclusiveSelector>() : make_shared<Selector>();
}

shared_ptr<Sequence> Foundation::sequence(){
	return make_shared<Sequence>();
}

shared_ptr<Loop> Foundation::loop(){
	return make_shared<Loop>();
}

Literal::Literal(const string& lit) : mLiteral(tolower(lit)), mLiteralSize(mLiteral.size()) {

}

size_t Literal::_feed(const shared_ptr< ParserContextBase >& ctx, const string& input, size_t pos){
	size_t i;
	for(i=0;i<mLiteralSize;++i){
		if (::tolower(input[pos+i])!=mLiteral[i]) return string::npos;
	}
	return mLiteralSize;
}

void Literal::_optimize(int recursionLevel){

}

bool Literal::_getTransitionMap(TransitionMap* mask){
	mask->mPossibleChars[::tolower(mLiteral[0])]=true;
	mask->mPossibleChars[::toupper(mLiteral[0])]=true;
	return true;
}

void Literal::save(std::ofstream& outFile, long &savePos){

			outFile.seekp(savePos, ios::beg);

			if(outFile){

				outFile << "LIT" << " " << mLiteral  << " " ;
				savePos = outFile.tellp();
			}
			else{
				cerr << "error writing to file : ecriture dans le fichier impossibe" << endl;
			}
	}



shared_ptr<Literal> Literal::load(std::ifstream& inFile, long &loadPos){

		inFile.seekg(loadPos, ios::beg);
		if(inFile){
			string mliteral_loaded;

			std::string tag = "";
			inFile >> tag ;

				if (tag == "LIT"){
					inFile >> mliteral_loaded;

					loadPos = inFile.tellg();
				}
				else{
					cerr << "ERROR : no LITERAL tag found" << endl;
				}
				return make_shared<Literal>(mliteral_loaded);
			}
		else{
			cerr << "ERROR : file stream unavailable" << endl;
		}
		return NULL;
	}


shared_ptr<Literal> Literal::loadVect(std::vector<string>::const_iterator &inIter){
//  cout << "DEBUG : tag : <" << *(inIter) << ">";

  if((*inIter) == "LIT"){
    string mliteral_loaded =  *(++inIter);
//    cout << " | string : <" << *(inIter) << ">"<< endl;
    ++inIter;
    return make_shared<Literal>(mliteral_loaded);
  }
	else{
  	cerr << "ERROR : no LITERAL tag found" << endl;
  }
  return NULL;
}



bool Literal::equal(const shared_ptr<Recognizer> &LIT){
		const shared_ptr<Literal> LIT_local = dynamic_pointer_cast<Literal>(LIT); //cast en type approprié pour pouvoir accéder aux attributs

		return(LIT_local->mLiteral == mLiteral) ? true : false;
	}

	//DEBUG FUNCTION
	void Literal::printtype(){
		cout << "type : Literal";
	}



shared_ptr<Recognizer> Utils::literal(const string & lt){
	return make_shared<Literal>(lt);
}

shared_ptr<Recognizer> Utils::char_range(int begin, int end){
	return make_shared<CharRange>(begin, end);
}

RecognizerPointer::RecognizerPointer() {
}

shared_ptr<Recognizer> RecognizerPointer::getPointed(){
	return mRecognizer;
}

size_t RecognizerPointer::_feed(const shared_ptr<ParserContextBase> &ctx, const string &input, size_t pos){
	if (mRecognizer){
		return mRecognizer->feed(ctx, input, pos);
	}else{
		cerr<<"RecognizerPointer with name '"<<mName<<"' is undefined"<<endl;
		abort();
	}
	return string::npos;
}

void RecognizerPointer::setPointed(const shared_ptr<Recognizer> &r){
	mRecognizer=r;
}


void RecognizerPointer::_optimize(int recursionLevel){
	/*do not call optimize() on the pointed value to avoid a loop.
	 * The grammar will do it for all rules anyway*/
}


//MODIFIE PAR IYED
void RecognizerPointer::save(std::ofstream& outFile, long &savePos){

		outFile.seekp(savePos, ios::beg);
		if(outFile){
			outFile << "RECP" << " ";
			mRecognizer->save(outFile, savePos);
		}
		else{
			cerr << "ERROR : file stream unavailable" << endl;
		}
}

//MODIFIE PAR IYED
shared_ptr<RecognizerPointer> RecognizerPointer::load(std::ifstream& inFile, long &loadPos){
	inFile.seekg(loadPos, ios::beg);
	shared_ptr<RecognizerPointer> recognpointer_reloaded = NULL; //type de retour

	if(inFile){


		std::string tag = "";
		inFile >> tag ;


			if (tag == "RECP"){
					loadPos = inFile.tellg();



					long pos_pre_tag = inFile.tellg();
					inFile >> tag;

					if (tag == "CR"){
							inFile.seekg(pos_pre_tag, ios::beg);
							const shared_ptr<CharRecognizer> mRecognizer_loaded = CharRecognizer::load(inFile, loadPos);
							recognpointer_reloaded = make_shared<RecognizerPointer>();
							recognpointer_reloaded->setPointed(mRecognizer_loaded); //houni errue

					}
					else if(tag == "SEL"){
						inFile.seekg(pos_pre_tag, ios::beg);
						const shared_ptr<Selector> mRecognizer_loaded = (Selector::load(inFile, loadPos));
						recognpointer_reloaded = make_shared<RecognizerPointer>();
						recognpointer_reloaded->setPointed(mRecognizer_loaded);
					}
					else if(tag == "XSEL"){
						inFile.seekg(pos_pre_tag, ios::beg);
						const shared_ptr<ExclusiveSelector> mRecognizer_loaded = (ExclusiveSelector::load(inFile, loadPos));
						recognpointer_reloaded = make_shared<RecognizerPointer>();
						recognpointer_reloaded->setPointed(mRecognizer_loaded);
					}
					else if(tag == "SEQ"){
						inFile.seekg(pos_pre_tag, ios::beg);
						const shared_ptr<Sequence> mRecognizer_loaded = (Sequence::load(inFile, loadPos));
						recognpointer_reloaded = make_shared<RecognizerPointer>();
						recognpointer_reloaded->setPointed(mRecognizer_loaded);
					}
					else if(tag == "LOOP"){
						inFile.seekg(pos_pre_tag, ios::beg);
						const shared_ptr<Loop> mRecognizer_loaded = (Loop::load(inFile, loadPos));
						recognpointer_reloaded = make_shared<RecognizerPointer>();
						recognpointer_reloaded->setPointed(mRecognizer_loaded);
					}
					else if(tag == "XCR"){
						inFile.seekg(pos_pre_tag, ios::beg);
						const shared_ptr<CharRange> mRecognizer_loaded = (CharRange::load(inFile, loadPos));
						recognpointer_reloaded = make_shared<RecognizerPointer>();
						recognpointer_reloaded->setPointed(mRecognizer_loaded);
					}
					else if(tag == "LIT"){
						inFile.seekg(pos_pre_tag, ios::beg);
						const shared_ptr<Literal> mRecognizer_loaded = (Literal::load(inFile, loadPos));
						recognpointer_reloaded = make_shared<RecognizerPointer>();
						recognpointer_reloaded->setPointed(mRecognizer_loaded);
					}
					else if(tag == "RECP"){
						inFile.seekg(pos_pre_tag, ios::beg);
						const shared_ptr<RecognizerPointer> mRecognizer_loaded = (RecognizerPointer::load(inFile, loadPos));
						recognpointer_reloaded = make_shared<RecognizerPointer>();
						recognpointer_reloaded->setPointed(mRecognizer_loaded);
					}
					else
							std::cerr << "Undefined Recognizer type" << '\n';


			}
			else{
				cerr << "no RECONGIZER PTR tag found" << endl;
			}
	}
	else{
		cerr << "lecture du fichier impossible" << endl;
	}
	return recognpointer_reloaded;
}

shared_ptr<RecognizerPointer> RecognizerPointer::loadVect(std::vector<string>::const_iterator &inIter){
//  cout << "DEBUG : tag : <" << *(inIter) << ">";
	shared_ptr<RecognizerPointer> recognpointer_reloaded = NULL; //type de retour

	if((*inIter) == "RECP"){
    ++inIter;
    recognpointer_reloaded = make_shared<RecognizerPointer>();
//    cout << " | taginside : <" << *(inIter) << ">" << endl;

					if ((*inIter) == "CR"){
							recognpointer_reloaded->setPointed(CharRecognizer::loadVect(inIter)); //houni errue
					}
					else if ((*inIter) == "SEL"){
							recognpointer_reloaded->setPointed(Selector::loadVect(inIter)); //houni errue
					}
					else if((*inIter) == "XSEL"){
            recognpointer_reloaded->setPointed(ExclusiveSelector::loadVect(inIter)); //houni errue
					}
					else if((*inIter) == "SEQ"){
            recognpointer_reloaded->setPointed(Sequence::loadVect(inIter)); //houni errue
					}
					else if((*inIter) == "LOOP"){
            recognpointer_reloaded->setPointed(Loop::loadVect(inIter)); //houni errue
					}
					else if((*inIter) == "XCR"){
            recognpointer_reloaded->setPointed(CharRange::loadVect(inIter)); //houni errue
					}
					else if((*inIter) == "LIT"){
            recognpointer_reloaded->setPointed(Literal::loadVect(inIter)); //houni errue
					}
					else if((*inIter) == "RECP"){
            recognpointer_reloaded->setPointed(RecognizerPointer::loadVect(inIter)); //houni errue
					}
					else
							std::cerr << "Undefined Recognizer type" << '\n';


			}
			else{
				cerr << "no RECONGIZER PTR tag found" << endl;
			}
	return recognpointer_reloaded;
}


bool RecognizerPointer::equal(const std::shared_ptr<Recognizer> &LOP){

	const shared_ptr<RecognizerPointer> LOP_local = dynamic_pointer_cast<RecognizerPointer>(LOP);
	return(mRecognizer->equal(LOP_local->mRecognizer)) ? true : false;
}

//DEBUG FUNCTION
void RecognizerPointer::printtype(){
	cout << "type : RECP";
	//DEBUG FUNCTION
					mRecognizer->printtype();
		}



Grammar::Grammar(const string& name) : mName(name){

}

Grammar::~Grammar() {
	for(auto it = mRecognizerPointers.begin(); it != mRecognizerPointers.end(); ++it) {
		shared_ptr<RecognizerPointer> pointer = dynamic_pointer_cast<RecognizerPointer>(*it);
		pointer->setPointed(NULL);
	}
}

void Grammar::assignRule(const string &argname, const shared_ptr<Recognizer> &rule){
	string name=tolower(argname);
	rule->setName(name);
	auto it=mRules.find(name);
	if (it!=mRules.end()){
		shared_ptr<RecognizerPointer> pointer=dynamic_pointer_cast<RecognizerPointer>((*it).second);
		if (pointer){
			pointer->setPointed(rule);
		}else{
			cerr<<"Error: rule '"<<name<<"' is being redefined !"<<endl;
			abort();
		}
	}
	/*in any case the map should contain real recognizers (not just pointers) */
	mRules[name]=rule;
}

void Grammar::_extendRule(const string &argname, const shared_ptr<Recognizer> &rule){
	string name=tolower(argname);
	rule->setName("");
	auto it=mRules.find(name);
	if (it!=mRules.end()){
		shared_ptr<Selector> sel=dynamic_pointer_cast<Selector>((*it).second);
		if (sel){
			sel->addRecognizer(rule);
		}else{
			cerr<<"Error: rule '"<<name<<"' cannot be extended because it was not defined with a Selector."<<endl;
			abort();
		}
	}else{
		cerr<<"Error: rule '"<<name<<"' cannot be extended because it is not defined."<<endl;
		abort();
	}
}

shared_ptr<Recognizer> Grammar::findRule(const string &argname){
	string name=tolower(argname);
	auto it=mRules.find(name);
	if (it!=mRules.end()){
		return (*it).second;
	}
	return NULL;
}

shared_ptr<Recognizer> Grammar::getRule(const string &argname){
	shared_ptr<Recognizer> ret=findRule(argname);

	if (ret){
		shared_ptr<RecognizerPointer> pointer=dynamic_pointer_cast<RecognizerPointer>(ret);
		if (pointer){
			if (pointer->getPointed()){/*if pointer is defined return the pointed value directly*/
				return pointer->getPointed();
			}else{
				return pointer;
			}
		}
		return ret;
	}else{/*the rule doesn't exist yet: return a pointer*/
		shared_ptr<RecognizerPointer> recognizerPointer = make_shared<RecognizerPointer>();
		ret=recognizerPointer;
		string name=tolower(argname);
		ret->setName(string("@")+name);
		mRules[name]=ret;
		mRecognizerPointers.push_back(recognizerPointer);
	}
	return ret;
}

void Grammar::include(const shared_ptr<Grammar>& grammar){
	for(auto it=grammar->mRules.begin();it!=grammar->mRules.end();++it){
		if (mRules.find((*it).first)!=mRules.end()){
			cerr<<"Rule '"<<(*it).first<<"' is being redefined while including grammar '"<<grammar->mName<<"' into '"<<mName<<"'"<<endl;
		}
		mRules[(*it).first]=(*it).second;
	}
}

bool Grammar::isComplete()const{
	bool ret=true;
	for(auto it=mRules.begin(); it!=mRules.end(); ++it){
		shared_ptr<RecognizerPointer> pointer=dynamic_pointer_cast<RecognizerPointer>((*it).second);
		if (pointer && !pointer->getPointed()){
			cerr<<"Rule '"<<(*it).first<<"' is not defined."<<endl;
			ret=false;
		}
	}
	return ret;
}

void Grammar::optimize(){
	for(auto it=mRules.begin(); it!=mRules.end(); ++it){
		(*it).second->optimize();
	}
}


int Grammar::getNumRules() const{
	return (int)mRules.size();
}

void Grammar::saveRulesMap(ofstream &outFile){
	long grammarSavePosition = 0;
		if(outFile){
			for(map<string,shared_ptr<Recognizer>>::iterator it= mRules.begin(); it != mRules.end() ;it++){

				//saving the rule key
				outFile.seekp(grammarSavePosition, ios::beg);
				outFile << it->first << " => ";
			//	cout << "DEBUG : key : " << it->first << endl;//"|| value : "; (it->second)->printtype();
				grammarSavePosition = outFile.tellp();

				(it->second)->save(outFile, grammarSavePosition); //recognizer

		//		cout << "DEBUG : no seg fault yet : value ok" << endl;
				outFile << "\r\n";
			}
		}
		else{
			cerr << "ERROR writing to file : output stream unavailable" << endl;
		}
	}



std::map<std::string,std::shared_ptr<Recognizer>> Grammar::loadRulesMap(ifstream &inFile){


  long grammarLoadPosition = 0;
	string tempKey = "";
	string tempTag = "";
	shared_ptr<Recognizer> tempValue = NULL;
	std::map<std::string,std::shared_ptr<Recognizer>> mRulesLoaded;
	string separator = "";

		if(inFile){
		while(!inFile.eof()){

			inFile >> tempKey ;
			inFile >> separator;
		while (separator != "=>") {
			tempKey +=separator;
			inFile >> separator;
		}
			if(separator != "=>"){cerr << "ERROR reading file : file format non conform";}
			grammarLoadPosition = inFile.tellg();

			inFile >> tempTag;
			//cout << "DEBUG : key :" << tempKey;
			//cout << "|| separator : " << separator ;
			//cout << " || tag : " << tempTag << endl;
			if (inFile.eof()) break;

			if(tempTag == "CR"){tempValue = CharRecognizer::load(inFile,grammarLoadPosition);}
			if(tempTag == "SEL"){tempValue = Selector::load(inFile,grammarLoadPosition);}
			if(tempTag == "XSEL"){tempValue = ExclusiveSelector::load(inFile,grammarLoadPosition);}
			if(tempTag == "XCR"){tempValue = CharRange::load(inFile,grammarLoadPosition);}
			if(tempTag == "SEQ"){tempValue = Sequence::load(inFile,grammarLoadPosition);}
			if(tempTag == "LOOP"){tempValue = Loop::load(inFile,grammarLoadPosition);}
			if(tempTag == "LIT"){tempValue = Literal::load(inFile,grammarLoadPosition);}
			if(tempTag == "RECP"){tempValue = RecognizerPointer::load(inFile,grammarLoadPosition);}
			mRulesLoaded[tempKey] = tempValue;

		}
		}

		return mRulesLoaded;

	}

template<typename Out>
  void split(const std::string &s, char delim, Out result) {
      std::stringstream ss;
      ss.str(s);
      std::string item;
      while (std::getline(ss, item, delim)) {
          *(result++) = item;
      }
  }


  std::vector<std::string> split(const std::string &s, char delim) {
      std::vector<std::string> elems;
      split(s, delim, std::back_inserter(elems));
      return elems;
  }


shared_ptr<Grammar> Grammar::loadVectRulesMap(string fileName){

  string mNameLoaded = "";
  string separator = "";
  vector <string> fileStringRules; // vecteur contenant les regles de la grammaaire en format string

//  long grammarLoadPosition = 0;
  string tempKey = "";
  string tempTag = "";
  shared_ptr<Recognizer> tempValue = NULL;
  std::map<std::string,std::shared_ptr<Recognizer>> mRulesLoaded;


  ifstream inFile (fileName, ios::in | ios::binary);

  if ( !inFile )
    cout << "fichier inexistant";
  else{
//lecture du nom de la grammaire
//  cout << "INSIDE THA MATRIX : no seg fault yet" << endl;

    std::stringstream buffer; // variable contenant l'intégralité du fichier
    buffer << inFile.rdbuf();
    inFile.close();
    string stringBuffer = buffer.str(); // string contenant l'integralité du fichier
    std::vector<std::string> fileStringRules = split(stringBuffer, '\n'); // vecteur ou chaque ligne est une ligne du fichier
  //  cout << "DEBUG : file :<" << stringBuffer << ">;" << endl;

    vector<string>::const_iterator fileIter = fileStringRules.begin(); //iterateur
//    cout << "file parsed and into the vector : no seg fault yet" << endl;
    string line = (*fileIter);
  //  cout << "DEBUG : first line :<" << line << ">;" << endl;
    std::vector<std::string> result = split(line, ' ');
    vector<string>::const_iterator it = result.begin();

  //  cout << "DEBUG: first line parsed and into the vector : no seg fault yet" << endl;

    mNameLoaded = *(it);
    separator = *(++it);
    while (separator != "==>") {
      mNameLoaded += ( " " + separator);
      separator = *(++it);
    //  cout << "DEBUG : sepp" << separator << endl;
    }

//  cout << "DEBUG : file opened HAMDOULAH" << endl << "DEBUG : grammar name : <" << mNameLoaded << "> | <" << separator << ">"<<  endl;
  ++fileIter;
  //  bool continuer = true;
//lecture des regles de la grammaire
    while( (*fileIter) != "" )
    {
      //  cout << "DEBUG : line parsed : <" << *fileIter << ">;" <<  endl;
        result = split(*fileIter, ' ');
        it = result.begin();
        tempKey = *it;
        separator = *(++it);
        while (separator != "=>") {
          tempKey +=separator;
          separator = *(++it);
        }
        tempTag = *(++it);
        if(tempTag == "CR"){tempValue = CharRecognizer::loadVect(it);}
  			if(tempTag == "SEL"){tempValue = Selector::loadVect(it);}
  			if(tempTag == "XSEL"){tempValue = ExclusiveSelector::loadVect(it);}
  			if(tempTag == "XCR"){tempValue = CharRange::loadVect(it);}
  			if(tempTag == "SEQ"){tempValue = Sequence::loadVect(it);}
  			if(tempTag == "LOOP"){tempValue = Loop::loadVect(it);}
  			if(tempTag == "LIT"){tempValue = Literal::loadVect(it);}
  			if(tempTag == "RECP"){tempValue = RecognizerPointer::loadVect(it);}
        mRulesLoaded[tempKey] = tempValue;
        ++fileIter;
    }
  }
  inFile.close();

  shared_ptr<Grammar> retGram = make_shared<Grammar>(mNameLoaded);
  retGram->mRules = mRulesLoaded;
  return retGram;


}

void Grammar::createGrammarDump(string dumpFileName){
	ofstream outFile (dumpFileName, ios::out | ios::app | ios::binary);
	outFile << mName << " ==> "<<endl;
	this->saveRulesMap(outFile);
	outFile.close();
}

shared_ptr<Grammar> Grammar::createLoadedGrammar(string fileName){
	//The created grammar has the name of the file containing its rules
	string mNameLoaded = "";
	string separator = "";
	ifstream inFile (fileName, ios::in | ios::binary);
	inFile >> mNameLoaded;
	inFile >> separator;
	while (separator != "==>") {
		mNameLoaded += (" " +separator);
		inFile >> separator;
	}

	std::map<std::string,std::shared_ptr<Recognizer>> mRulesLoaded = Grammar::loadRulesMap(inFile);
	inFile.close();

	shared_ptr<Grammar> retGram = make_shared<Grammar>(mNameLoaded);
	retGram->mRules = mRulesLoaded;
	return retGram;
}

bool Grammar::equal(shared_ptr<Grammar> &gramCompared){
		bool condition = true;

		if(mName != gramCompared->mName){condition = false;}
	//	cout << "DEBUG :source name : "<< mName << " target name : " << gramCompared->mName << endl;
		map<string,shared_ptr<Recognizer>>::iterator it= mRules.begin();
		map<string,shared_ptr<Recognizer>>::iterator itComp= gramCompared->mRules.begin();

		while (it != mRules.end() && condition){
		/*		cout << "DEBUG : first element key : " << it->first << "|| second element key : " << itComp->first << endl;
				cout << "DEBUG : first element value: " ; (it->second)->printtype(); cout << endl;
				cout<< "DEBUG : second element value: "; (itComp->second)->printtype();*/
				if(it->first == itComp->first){
					if(!(it->second)->equal(itComp->second)){
	//					cout << "DEBUG : diffrent shit" << endl;
						condition = false;
					}

				}
				else{
					condition = false;
				}
			it++;
			itComp++;
		}
	return condition;
}

void Grammar::debugGrammar(){
	for(map<string,shared_ptr<Recognizer>>::iterator it= mRules.begin(); it != mRules.end() ;it++){
		cout << "DEBUG : key "<<it->first << " ";
		(it->second)->printtype();
		cout << " " << endl;
}
}



string belr::tolower(const string &str){
	string ret(str);
	transform(ret.begin(),ret.end(), ret.begin(), ::tolower);
	return ret;
}
