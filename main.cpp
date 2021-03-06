
#include <iostream>
#include <string.h>
#include <locale.h>

#include <vector>
#include <map>
#include <stdlib.h>
#include <assert.h>
#include <sstream>

#include <fstream>
#include <math.h>
#include <algorithm>
#include <set>
#include <memory>

#include "BayesClassifier.h"
#include "LexerStem.h"
#include "LexerMyStem.h"

using namespace std;

typedef struct {
		unsigned int counter;
		string word;
	} WordInfo;
typedef map <Hash32, WordInfo> WordList;


int learn(string file, lexer * Lexer) {

    UErrorCode status = U_ZERO_ERROR;
    RegexMatcher matcher("([a-zа-я])+", 0, status);
    RegexMatcher header("^----class ([\\d]+) -----$", 0, status);

    WordList words_all;
    ClassifierList classifier;
    ClassifierList::iterator words = classifier.end();

    while (!cin.eof()) {
        string buf;
        std::getline(cin, buf);
        UnicodeString ucs = UnicodeString::fromUTF8(buf.c_str());
		header.reset(ucs);
        if (header.find()) {
            if (words != classifier.end()) {
                Lexer->parse_end();
                Lexer->word_stat(words->second.words);
            }
        	string str;
        	header.group(1,status).toUTF8String(str);
        	int cl = atoi(str.c_str());
        	if (classifier.find(cl) == classifier.end()) {
        		ClassifierStat stat;
        		stat.docs_count = 0;
        		classifier[cl] = stat;
        	}
			words = classifier.find(cl);
			words->second.docs_count ++;
			Lexer->parse_begin();
			continue;
        }

        assert(words != classifier.end());

        ucs.toLower();
        Lexer->parse(ucs);
        Lexer->word_stat(words->second.words);
    }

    Lexer->parse_end();
    Lexer->word_stat(words->second.words);

	BayesClassifier bayes;
	bayes.setStats(classifier);

	ofstream ofs(file.c_str());
	bayes.saveToStream(ofs);
	ofs.close();
/*
	for(WordList::const_iterator item = words_all.begin(); item != words_all.end(); item++) {
		cout << item->first << " " << item->second.word << " " << item->second.counter << endl;
	}

	for(ClassifierList::const_iterator classificator = classifier.begin(); classificator!= classifier.end(); classificator++) {
		cout << "-----CLASSIFICATOR " << classificator->first << " --------" << endl;
		for(WordsStat::const_iterator stats = classificator->second.begin(); stats != classificator->second.end(); stats++)
			cout << stats->first << "\t" << stats->second << endl;
	}
*/
	return 0;
}

int classifier(string file, lexer * Lexer) {

	BayesClassifier bayes;
	ifstream ifs(file.c_str());

	if (!ifs) {
	    cerr << "Файл не найден: " << file << endl;
	    exit(1);
	}

	bayes.loadFromStream(ifs);
	ifs.close();

    UErrorCode status = U_ZERO_ERROR;
    RegexMatcher matcher("([a-zа-я])+", 0, status);

    ClassifierList classifier;
    WordsStat words;

    while (!cin.eof()) {

		stringstream text;
		char sym;

		do {
			sym = cin.get();
			if (sym == 0) break;
			text << sym;
		} while (cin.good());
        Lexer->parse_begin();

		while (!text.eof()) {
			string buf;

			getline(text, buf);

			//cout << "buf: "<<buf << endl;

			UnicodeString ucs = UnicodeString::fromUTF8(buf);

			ucs.toLower();

			Lexer->parse(ucs);
            Lexer->word_stat(words);
        }



        Lexer->parse_end();
        Lexer->word_stat(words);

		ClassifierP p;

		ClassifierP answer = bayes.classify(words);
		//float k = (std::max_element(answer.begin(), answer.end()))->second;

		for(ClassifierP::const_iterator i = answer.begin(); i != answer.end(); i ++) {
			float sum = 0;
			for(ClassifierP::const_iterator j = answer.begin(); j != answer.end(); j++ ) {
				sum += exp( j->second - i->second );
			}
			p[i->first] = 1/ sum;
		}


		for(ClassifierP::const_iterator item = answer.begin(); item != answer.end(); item++) {
			cout << item->first << ' ' << item->second << ' ' << p[item->first] << endl;
		}
		cout << endl;
		words.clear();
    }

	return 0;

}

int main(int argc, char* argv[]) {
    if (!setlocale(LC_ALL, "ru_RU.utf8"))
        if (!setlocale(LC_ALL, "russian")) {
            cerr << "Не могу установить русскую локаль: ru_RU.utf8";
            return 1;
        }

    char *path = new char[strlen(argv[0])];
    strcpy(path, argv[0]);

    *(strrchr(path, '/') + 1) = 0; // Убираем имя программы
    string program_path = path;
    delete [] path;

	if (argc < 3) {
		cout <<
"Классификатор текста на основе классификатора Байеса (ver 0.04) \n"
"bayes [режим_работы] [имя_файла_конфигурации] [тип_лексера] [фильтр_лексем для лексера (mystem) ]\n"
"РЕЖИМЫ РАБОТЫ: \n"
" - L - обучение (после обучения данные будут записаны в файл конфигурации) \n"
"\t На stdin подается набор (много штук) -----CLASSIFICATOR {uint_классификатора} --------\\n + \n"
"\t текст для классификации. Где uint_классификатора -- класс к которому должен принадлежать текст во время классификации \n"
"\t После закрытия stdin классификатор формирует файл конфигурации. \n"
" - C - классификация. На stdin подаем классифицируемый текст с \\0 в конце. На stdout получаем численные оценки принадлежности c \\0 в конце. \n"
"\n"
"ТИПЫ ЛЕКСЕРОВ: \n"
"\t stem (по-умолчанию) - выделение слов, стемминг для русского и английского. \n"
"\t mystem - работа ведется через программу mystem (Яндекс) (mystem2 - лексемы будут составляться из пар слов, mystem3 - из троек слов) \n"
"\n\n Логунцов С.В. 2013 г. mailto: loguntsov@gmail.com \n";
		return 0;
	}

	string file(argv[2]);


	string mode(argv[1]);

    lexer * Lexer = NULL;

	if (argc > 3) {
        string match = "";
        if (argc > 4) match = argv[4];

        string lexer(argv[3]);

        if (lexer == "mystem") {
            Lexer = new LexerMyStem(program_path, match);
        } else
        if (lexer == "mystem2") {
            LexerMyStem *a = new LexerMyStem(program_path, match);
            a->queue_size = 2;
            Lexer = a;
        } else
        if (lexer == "mystem3") {
            LexerMyStem *a = new LexerMyStem(program_path, match);
            a->queue_size = 3;
            Lexer = a;
        } else

        if (lexer == "stem") {
            Lexer = new LexerStem();
        }
        if (Lexer == NULL) {
            cerr << "Неизвестный тип лексера. Доступные варианты: mystem, stem";
            return 1;
        }
	}

    if (Lexer == NULL) {
        Lexer = new LexerStem();
    }

	switch(argv[1][0]) {
		case 'L' :
			return learn(file, Lexer);
		case 'C' :
			return classifier(file, Lexer);
		break;
	}

	delete Lexer;

    return 0;
}
