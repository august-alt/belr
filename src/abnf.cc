#include "abnf.hh"

namespace belr{

/*
 * CoreRules grammar
**/
CoreRules::CoreRules() : Grammar("core rules"){
	alpha();
	bit();
	char_();
	cr();
	lf();
	crlf();
	ctl();
	digit();
	hexdig();
	dquote();
	htab();
	octet();
	sp();
	vchar();
	wsp();
	lwsp();
}

void CoreRules::alpha(){
	shared_ptr<Selector> selector=make_shared<Selector>();
	
	selector->addRecognizer(Utils::char_range('a','z'));
	selector->addRecognizer(Utils::char_range('A','Z'));

	addRule("alpha",selector);
}

void CoreRules::bit(){
	shared_ptr<Selector> selector=make_shared<Selector>();
	selector->addRecognizer(make_shared<CharRecognizer>('0'));
	selector->addRecognizer(make_shared<CharRecognizer>('1'));
	addRule("bit",selector);
}

void CoreRules::char_(){
	addRule("char",Utils::char_range(0x1,0x7f));
}

void CoreRules::cr(){
	addRule("cr", Foundation::charRecognizer(0x0d,true));
}

void CoreRules::lf(){
	addRule("lf",Foundation::charRecognizer(0x0a,true));
}

void CoreRules::crlf(){
	addRule("crlf", Foundation::sequence()
		->addRecognizer(getRule("cr"))
		->addRecognizer(getRule("lf")));
}

void CoreRules::ctl(){
	addRule("ctl",
		Foundation::selector()
			->addRecognizer(Utils::char_range(0x00, 0x1f))
			->addRecognizer(Foundation::charRecognizer(0x7f,true))
	);
}

void CoreRules::digit(){
	addRule("digit",Utils::char_range(0x30,0x39));
}

void CoreRules::dquote(){
	addRule("dquote",Foundation::charRecognizer(0x22,true));
}

void CoreRules::hexdig(){
	addRule("hexdig", Foundation::selector()
		->addRecognizer(getRule("digit"))
		->addRecognizer(Foundation::charRecognizer('A'))
		->addRecognizer(Foundation::charRecognizer('B'))
		->addRecognizer(Foundation::charRecognizer('C'))
		->addRecognizer(Foundation::charRecognizer('D'))
		->addRecognizer(Foundation::charRecognizer('E'))
		->addRecognizer(Foundation::charRecognizer('F'))
	);
}

void CoreRules::htab(){
	addRule("htab",Foundation::charRecognizer(0x09,true));
}

void CoreRules::octet(){
	addRule("octet",Utils::char_range(0,0xff));
}

void CoreRules::sp(){
	addRule("sp",Foundation::charRecognizer(0x20,true));
}

void CoreRules::vchar(){
	addRule("vchar", Utils::char_range(0x21, 0x7e));
}

void CoreRules::wsp(){
	addRule("wsp", Foundation::selector()
		->addRecognizer(getRule("sp"))
		->addRecognizer(getRule("htab"))
	);
}

void CoreRules::lwsp(){
	addRule("lwsp", Foundation::loop()->setRecognizer(Foundation::selector()
		->addRecognizer(getRule("wsp"))
		->addRecognizer(Foundation::sequence()
			->addRecognizer(getRule("crlf"))
			->addRecognizer(getRule("wsp"))
			)
		)
	);
}

/*
 * ABNF grammar
**/

ABNFGrammar::ABNFGrammar(): Grammar("ABNF"){
	include(make_shared<CoreRules>());
	char_val();
	bin_val();
	dec_val();
	hex_val();
	num_val();
	prose_val();
	comment();
	c_nl();
	c_wsp();
	rulename();
	repeat();
	defined_as();
	rulelist();
	rule();
	elements();
	alternation();
	concatenation();
	repetition();
	element();
	group();
	option();
}

void ABNFGrammar::comment(){
	addRule("comment", Foundation::sequence()
		->addRecognizer(Foundation::charRecognizer(';',true))
		->addRecognizer(
			Foundation::loop()->setRecognizer(
				Foundation::selector()
					->addRecognizer(getRule("wsp"))
					->addRecognizer(getRule("vchar"))
				)
			)
		->addRecognizer(getRule("crlf"))
	);
}

void ABNFGrammar::c_nl(){
	addRule("c-nl", Foundation::selector()
		->addRecognizer(getRule("comment"))
		->addRecognizer(getRule("crlf")));
}

void ABNFGrammar::c_wsp(){
	addRule("c-wsp",Foundation::selector()
		->addRecognizer(getRule("wsp"))
		->addRecognizer(Foundation::sequence()
			->addRecognizer(getRule("c-nl"))->addRecognizer(getRule("wsp"))
		)
	);
}

/* ALPHA *(ALPHA / DIGIT / "-")*/
void ABNFGrammar::rulename(){
	addRule("rulename", Foundation::sequence()
		->addRecognizer(getRule("alpha"))
		->addRecognizer(Foundation::loop()->setRecognizer(
			Foundation::selector()->addRecognizer(getRule("alpha"))
					->addRecognizer(getRule("digit"))
					->addRecognizer(Foundation::charRecognizer('-'))
			)
		)
	);
}

/* 1*DIGIT / (*DIGIT "*" *DIGIT) */
void ABNFGrammar::repeat(){
	addRule("repeat", Foundation::selector()
		->addRecognizer(
			Foundation::loop()->setRecognizer(getRule("digit"),1)
		)
		->addRecognizer(Foundation::sequence()
			->addRecognizer(
				Foundation::loop()->setRecognizer(getRule("digit"))
			)
			->addRecognizer(
				Foundation::charRecognizer('*')
			)
			->addRecognizer(
				Foundation::loop()->setRecognizer(getRule("digit"))
			)
		)
	);
}

/* *c-wsp ("=" / "=/") *c-wsp */
void ABNFGrammar::defined_as(){
	addRule("defined-as", Foundation::sequence()
		->addRecognizer(
			Foundation::loop()->setRecognizer(getRule("c-wsp"))
		)
		->addRecognizer(
			Foundation::selector()
				->addRecognizer(Foundation::charRecognizer('='))
				->addRecognizer(Utils::literal("=/"))
		)
		->addRecognizer(
			Foundation::loop()->setRecognizer(getRule("c-wsp"))
		)
	);
}

/*  1*( rule / (*c-wsp c-nl) ) */
void ABNFGrammar::rulelist(){
	addRule("rulelist", Foundation::loop()
		->setRecognizer(Foundation::selector()
			->addRecognizer(getRule("rule"))
			->addRecognizer(
				Foundation::sequence()
					->addRecognizer(
						Foundation::loop()->setRecognizer(getRule("c-wsp"))
					)
					->addRecognizer(getRule("c-nl"))
			)
		,1)
	);
}

/*  rulename defined-as elements c-nl  */
void ABNFGrammar::rule(){
	addRule("rule", Foundation::sequence()
		->addRecognizer(getRule("rulename"))
		->addRecognizer(getRule("defined-as"))
		->addRecognizer(getRule("elements"))
		->addRecognizer(getRule("c-nl"))
	);
}

/* alternation *c-wsp */
void ABNFGrammar::elements(){
	addRule("elements",Foundation::sequence()
		->addRecognizer(getRule("alternation"))
		->addRecognizer(
			Foundation::loop()->setRecognizer(getRule("c-wsp"))
		)
	);
}

/*
alternation    =  concatenation
                          *(*c-wsp "/" *c-wsp concatenation)
                          */
void ABNFGrammar::alternation(){
	addRule("alternation", Foundation::sequence()
		->addRecognizer(getRule("concatenation"))
		->addRecognizer(
			Foundation::loop()
				->setRecognizer(
					Foundation::sequence()
						->addRecognizer(
							Foundation::loop()->setRecognizer(getRule("c-wsp"))
						)
						->addRecognizer(
							Foundation::charRecognizer('/')
						)
						->addRecognizer(
							Foundation::loop()->setRecognizer(getRule("c-wsp"))
						)
						->addRecognizer(getRule("concatenation"))
				)
		)
	);
}

/*  concatenation  =  repetition *(1*c-wsp repetition) */
void ABNFGrammar::concatenation(){
	addRule("concatenation", Foundation::sequence()
		->addRecognizer(getRule("repetition"))
		->addRecognizer(
			Foundation::loop()->setRecognizer(
				Foundation::sequence()
					->addRecognizer(
						Foundation::loop()->setRecognizer(getRule("c-wsp"),1)
					)
					->addRecognizer(
						getRule("repetition")
					)
			)
		)
	);
}

/* repetition     =  [repeat] element */
void ABNFGrammar::repetition(){
	addRule("repetition", Foundation::sequence()
		->addRecognizer(Foundation::loop()->setRecognizer(getRule("repeat"),0,1))
		->addRecognizer(getRule("element"))
	);
}

/*
 * element        =  rulename / group / option /
 *                        char-val / num-val / prose-val
 */
void ABNFGrammar::element(){
	addRule("element", Foundation::selector()
		->addRecognizer(getRule("rulename"))
		->addRecognizer(getRule("group"))
		->addRecognizer(getRule("option"))
		->addRecognizer(getRule("char-val"))
		->addRecognizer(getRule("num-val"))
		->addRecognizer(getRule("prose-val"))
	);
}

/* 
    "(" *c-wsp alternation *c-wsp ")" 
 */
void ABNFGrammar::group(){
	addRule("group", Foundation::sequence()
		->addRecognizer(Foundation::charRecognizer('('))
		->addRecognizer(
			Foundation::loop()->setRecognizer(getRule("c-wsp"))
		)
		->addRecognizer(getRule("alternation"))
		->addRecognizer(
			Foundation::loop()->setRecognizer(getRule("c-wsp"))
		)
		->addRecognizer(Foundation::charRecognizer(')'))
	);
}

/*
 "[" *c-wsp alternation *c-wsp "]"
*/
void ABNFGrammar::option(){
	addRule("option", Foundation::sequence()
		->addRecognizer(Foundation::charRecognizer('['))
		->addRecognizer(Foundation::loop()->setRecognizer(getRule("c-wsp")))
		->addRecognizer(getRule("alternation"))
		->addRecognizer(Foundation::loop()->setRecognizer(getRule("c-wsp")))
		->addRecognizer(Foundation::charRecognizer(']'))
	);
}

/*
 * DQUOTE *(%x20-21 / %x23-7E) DQUOTE
**/
void ABNFGrammar::char_val(){
	addRule("char-val", Foundation::sequence()
		->addRecognizer(getRule("dquote"))
		->addRecognizer(
			Foundation::loop()->setRecognizer(
				Foundation::selector()
					->addRecognizer(Utils::char_range(0x20,0x21))
					->addRecognizer(Utils::char_range(0x23,0x7e))
			)
		)
		->addRecognizer(getRule("dquote"))
	);
}

/*
 * num-val        =  "%" (bin-val / dec-val / hex-val)
 */
void ABNFGrammar::num_val(){
	addRule("num-val", Foundation::sequence()
		->addRecognizer(Foundation::charRecognizer('%'))
		->addRecognizer(
			Foundation::selector()
				->addRecognizer(getRule("bin-val"))
				->addRecognizer(getRule("dec-val"))
				->addRecognizer(getRule("hex-val"))
		)
	);
}

/*
 * prose-val      =  "<" *(%x20-3D / %x3F-7E) ">"
 */
void ABNFGrammar::prose_val(){
	shared_ptr<Sequence> seq=make_shared<Sequence>();
	addRule("prose-val", Foundation::sequence()
		->addRecognizer(Foundation::charRecognizer('<'))
		->addRecognizer(
			Foundation::loop()->setRecognizer(
				Foundation::selector()
					->addRecognizer(Utils::char_range(0x20,0x3d))
					->addRecognizer(Utils::char_range(0x3f,0x7e))
			)
		)
		->addRecognizer(Foundation::charRecognizer('>'))
	);
}

/*
 * "b" 1*BIT
                          [ 1*("." 1*BIT) / ("-" 1*BIT) ]
 */
void ABNFGrammar::bin_val(){
	addRule("bin-val", Foundation::sequence()
		->addRecognizer(Foundation::charRecognizer('b'))
		->addRecognizer(
			Foundation::loop()->setRecognizer(getRule("bit"),1)
		)
		->addRecognizer(
			Foundation::loop()->setRecognizer(
				Foundation::selector()
					->addRecognizer(Foundation::loop()
						->setRecognizer(Foundation::sequence()
							->addRecognizer(
								Foundation::charRecognizer('.')
							)
							->addRecognizer(
								Foundation::loop()->setRecognizer(
								getRule("bit"), 1
								)
							)
						)
					)
					->addRecognizer(Foundation::sequence()
						->addRecognizer(Foundation::charRecognizer('-'))
						->addRecognizer(
							Foundation::loop()->setRecognizer(
								getRule("bit"), 1
							)
						)
					)
				,0,1
			)
		)
	);
}

/*
dec-val        =  "d" 1*DIGIT
                          [ 1*("." 1*DIGIT) / ("-" 1*DIGIT) ]
*/

void ABNFGrammar::dec_val(){
	addRule("dec-val", Foundation::sequence()
		->addRecognizer(Foundation::charRecognizer('d'))
		->addRecognizer(
			Foundation::loop()->setRecognizer(getRule("digit"),1)
		)
		->addRecognizer(
			Foundation::loop()->setRecognizer(
				Foundation::selector()
					->addRecognizer(Foundation::loop()
						->setRecognizer(Foundation::sequence()
							->addRecognizer(
								Foundation::charRecognizer('.')
							)
							->addRecognizer(
								Foundation::loop()->setRecognizer(
									getRule("digit"), 1
								)
							)
						)
					)
					->addRecognizer(Foundation::sequence()
						->addRecognizer(Foundation::charRecognizer('-'))
						->addRecognizer(
							Foundation::loop()->setRecognizer(
								getRule("digit"), 1
							)
						)
					)
				,0,1
			)
		)
	);
}

/*
 *  hex-val        =  "x" 1*HEXDIG
                          [ 1*("." 1*HEXDIG) / ("-" 1*HEXDIG) ]
*/
void ABNFGrammar::hex_val(){
	addRule("hex-val", Foundation::sequence()
		->addRecognizer(Foundation::charRecognizer('x'))
		->addRecognizer(
			Foundation::loop()->setRecognizer(getRule("hexdig"),1)
		)
		->addRecognizer(
			Foundation::loop()->setRecognizer(
				Foundation::selector()
					->addRecognizer(Foundation::loop()
						->setRecognizer(Foundation::sequence()
							->addRecognizer(
								Foundation::charRecognizer('.')
							)
							->addRecognizer(
								Foundation::loop()->setRecognizer(
									getRule("hexdig"), 1
								)
							)
						)
					)
					->addRecognizer(Foundation::sequence()
						->addRecognizer(Foundation::charRecognizer('-'))
						->addRecognizer(
							Foundation::loop()->setRecognizer(
								getRule("hexdig"), 1
							)
						)
					)
				,0,1
			)
		)
	);
}

}//end of namespace

