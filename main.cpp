#include <iostream>
#include <fstream>
#include <string>
#include <unordered_set>
#include <cstring>
#include <algorithm>
#include <cctype>
#include <stack>
#include <sstream>
#include <vector>

using namespace std;
//this is the set of declared variables and also evaluated temps are put here because it is convenient for implementation.
unordered_set <string> declaredVariables;
//we create lots of temporary variables through the project ,so we keep the total number of temporary variables as an integer so that we can assign unique temps.
int numOfTempVariables=1;
//to print errors with the line number the error occured ,we keeps the line number variable
int lineNum=0;
//To assign unique temporary variables to each condition, we kept the total number of while and if statements.
int whileCount=1;
//string that keeps almost everything we write into the output file,it starts from the declarations and does not cover the beginning part
string toOutfile="";
// we created a variable that we can use anywhere to decide if there is an error.Even if there is only one error it is changed to true.
//After reading each line we check if error is true ,if so we print Syntax Error + line number where the error has occured.
//Also if error is true,an error has been already detected some functions does not bother to do the needless work and just returns "error" strng.
bool error=false;

//some functions declared above for usage
string evaluate(string postfix, ofstream& outfile);
string term(string str, ofstream& outfile);
string moreTerms(string str, ofstream& outfile);
string moreFactors(string str, ofstream& outfile);
string factor(string str, ofstream& outfile);
void allocaAndStore( string variable);
string removeSpaces(string str);
bool tokenCheck(string& token);

 // arranges the line
string deletedoublespace (string line){


	if((line.length()>1 &&line.find("  ")!=string::npos)){

	}
	int doublespace=-1;
    if(line.find("#")!=string::npos){    // if there is a comment it ignores the rest
        line=line.substr(0,line.find("#"));
    }             //deletes one of the consecutive blank spaces
    while((line.length()>1 &&line.find("  ")!=string::npos)|| line.find_first_of("\t\n\v\f\r")!=string::npos){   // delete double spaces
        if(line.length()>1 &&line.find("  ")!=string::npos){
        	doublespace=line.find("  ");
        	line=line.substr(0,doublespace)+line.substr(doublespace+1,line.length()-(doublespace+1));
        }else{    // if there are characters implying blank spaces or new line etc. it replaces them with a blank space
        	doublespace=line.find_first_of("\t\n\v\f\r");
        	line= line.substr(0, doublespace) + " " + line.substr(doublespace+1, line.length()- doublespace-1);
        }

    }
    // also deletes blank spaces at the end of the expression.
    if(!line.empty()&&line.at(0)==' ') line=line.substr(1,line.length()-1); // delete blank spaces in the beginning and the end
    if(!line.empty()&& line.at(line.length()-1)==' ') line=line.substr(0,line.length()-1);
      // returns a string without double spaces, chars implying blank space ,blank spaces at the end and the beginning ,comments
    return line;
}
//prints error message to the output file
void printError(ofstream& outfile){
	outfile << "call i32 (i8*, ...)* @printf(i8* getelementptr ([23 x i8]* @printerror.str, i32 0, i32 0), i32 "+to_string(lineNum) + " )" << endl;
}

//this function checks whether a string is appropriate to be a varibale in our language
//it should start with a letter and should be alphanumerical.
bool variablecheck(string variable){
	//thisis our design choice, variables cannot be named if,while,print or choose(but can be named whilea, ify )
	if(variable=="if"|| variable=="while"|| variable=="print" || variable=="choose" ){
			return false;
	}
	// returns false if the string does not start with a char of the alphabet or the string contains a non-alphanumerical character.
	string alphabet="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	bool check=true;
	if(variable.find_first_not_of(alphabet)==string::npos){
		char initialLetterOfLeft= variable[0];
		string s="0123456789";
		if(s.find(initialLetterOfLeft)!=string::npos){
			return false;
		}else{

			return true;
		}
	}else{
		return false;
	}

}
//Returns the index of the next operator according to context.
//We only call it when op= "+-" or op = "*/" .
int nextWhatever(string str, string op){

	if(str.find(op[0])==string::npos){
		return str.find(op[1]);
	}
	if(str.find(op[1])==string::npos){
		return str.find(op[0]);
	}
	int firstp= str.find(op[0]);
	int firstm= str.find(op[1]);
	if(firstm<firstp){
		return firstm;
	}else{
		return  firstp;
	}
}

//It replaces inside of the paranthesis with empty spaces so that we can discard the expressions and operators inside of the parenthesis.
string discardParanthesis(string str){
	string toReturn=str;
	int left= 0;
	for(int i=0;i<toReturn.size();i++){
		if(left>0 && toReturn[i]!= '(' && toReturn[i]!= ')'){
			toReturn.replace(i, 1, " ");
		}else if(toReturn[i]=='('){
			left++;
		}else if(toReturn[i]==')'){
			left--;
		}
	}
	if(left!=0){  // controls if the parenthesis are valid(each open par matches wit a appropriate closed par)
        error=1;   // handles such cases: () ( ()
	}
	return toReturn;
}
//Returns postfix expression by calling functions that converts an expression to postfix
//If there are + or - signs returns "term + moreTerms", else returns "term"
string expression(string str, ofstream& outfile){
	string forSearch= discardParanthesis(str);
	if(error)return "error";
	if(forSearch.find("+")!=string::npos|| forSearch.find("-")!= string::npos){
		int opIndex= nextWhatever(forSearch, "+-");//Finds the index of first of (+or-)
		string firstTerm= str.substr(0,opIndex); // thanks to the nextWhatever and splits the string into two, first term and the rest
		string rest= str.substr(opIndex, str.size()- opIndex);
		return term(firstTerm, outfile)+" "+ moreTerms(rest, outfile); //returns the concatenation of two strings returned by term(first term)
		// and moreTerms(the rest of the expression).

	}else{
		return term(str, outfile);
	}
}
//If there are * or / returns "factor + moreFactors" , else returns "factor"
string term(string str, ofstream& outfile){
	string forSearch= discardParanthesis(str);
	if(forSearch.find("*")!= string::npos || forSearch.find("/")!= string::npos){
		int opIndex= nextWhatever(forSearch, "*/");
		string firstFactor= str.substr(0,opIndex);
		string rest= str.substr(opIndex);
		string s=factor(firstFactor, outfile) +" "+ moreFactors(rest, outfile);
		return s;
	}else{
		return factor(str, outfile);
	}

}
//If there are still more +/- signs returns term +(operator that used earlier) + moreTerms, else returns term + operator
string moreTerms(string str, ofstream& outfile){
	//operator will be written to postfix
	char op= str.at(0);
	str= str.substr(1);
	string forSearch= discardParanthesis(str);
	if(forSearch.find("+")!= string::npos || forSearch.find("-")!= string::npos){
		int opIndex= nextWhatever(forSearch, "+-");
		string firstTerm= str.substr(0,opIndex);
		string rest= str.substr(opIndex);
		return term(firstTerm, outfile) + " "  + op +" " + moreTerms(rest, outfile);

	}else{
		return term(str, outfile) + " "+ op;
	}

}
//same with moreTerms
string moreFactors(string str, ofstream& outfile){
	char op= str.at(0);
	str= str.substr(1);
	string forSearch= discardParanthesis(str);
	if(forSearch.find("*")!= string::npos || forSearch.find("/")!= string::npos){
		int opIndex= nextWhatever(forSearch, "*/");
		string firstTerm= str.substr(0,opIndex);
		string rest= str.substr(opIndex);
		return factor(firstTerm, outfile) + " "  + op +" " + moreFactors(rest, outfile);

	}else{
		return factor(str, outfile) + " "+ op;
	}


}

//it is for writing choose function inside the llvm, will be added before the main
void choose(ofstream& outfile){
	outfile << "define i32 @choose (i32 %input1, i32 %input2, i32 %input3, i32 %input4) {\n";
	outfile << "%cond1 = icmp eq i32 %input1, 0\n";
	outfile << "br i1 %cond1, label %IfEqual, label %IfUnequal\n";
	outfile << "IfEqual:\n";
	outfile << "ret i32 %input2\n";
	outfile << "IfUnequal:\n";
	outfile << "%cond2 = icmp sgt i32 %input1, 0\n";
	outfile << "br i1 %cond2, label %IfGreater, label %IfSmaller\n";
	outfile << "IfGreater:\n";
	outfile << "ret i32 %input3\n";
	outfile << "IfSmaller:\n";
	outfile << "ret i32 %input4\n";
	outfile << "}\n";

}
//choose should be written to llvm when program runs, this part is for calls
//this should calculate inputs of choose, then should call choose in llvm, and should return temp result of choose;
//add this temp to declared variables so it can be read after postfix
string returnedValueOfChoose(string str1, string str2, string str3, string str4,ofstream& outfile){
	string input1= evaluate(expression(str1, outfile), outfile);
	string input2= evaluate(expression(str2, outfile), outfile);
	string input3= evaluate(expression(str3, outfile), outfile);
	string input4= evaluate(expression(str4, outfile), outfile);

	string retTemp= "%t_" + to_string(numOfTempVariables);
	numOfTempVariables++;
	toOutfile += retTemp + " = call i32 @choose(i32 " + input1 +", i32 " + input2 + ", i32 "+input3+", i32 "+input4+")\n";
	declaredVariables.insert(retTemp);
	return retTemp;

}
//returns the factor's temp value or itself. Temp value is for choose function.
string factor(string str, ofstream& outfile){

	if(str.find("choose")!= string::npos){ //If it is a choose expression it controls the syntax
		string choToken= str.substr(0,str.find("("));
		string str1, str2, str3, str4;
		if(tokenCheck(choToken) && choToken=="choose"&&str.find("(")!=string::npos &&str.find_last_of(")")!=string::npos && str.find("(")<str.find_last_of(")")){
				int start= str.find("(")+1;
				int end= str.find_last_of(")");
				if(str.length()!=end+1){
					error=true;
					return "error";
				}

				string strLeft=str.substr(start, end-start);
				string check =discardParanthesis(str.substr(start, end-start));
				int count=0;
				string a= check;
   				 while(a.find(",")!=string::npos){
        			count++;
       				 a=a.substr(a.find(",")+1);
    			}
   				 if(a.length()==0 || count<3){
       				 error=true;
       				 return "Syntax Error";
       			}

				end= check.find(",");
				str1 = strLeft.substr(0,check.find(","));
				string left= check.substr(check.find(",")+1);
				strLeft= strLeft.substr(check.find(",")+1);
				str2= strLeft.substr(0, left.find(","));
				strLeft= strLeft.substr(left.find(",")+1);
				left= left.substr(left.find(",")+1);
				str3= strLeft.substr(0, left.find(","));
				strLeft= strLeft.substr(left.find(",")+1);
				left= left.substr(left.find(",")+1);
				str4= strLeft;      // if the expression is valid it calls choose functions

				return returnedValueOfChoose(str1,str2, str3, str4,outfile);

		}
		error=true;
		return "error";
		//else if there is valid parenthesis the returns expression function to examine inside of the parenthesis

	}
	else if(str.find("(")!=string::npos &&str.find_last_of(")")!=string::npos && str.find("(")< str.find_last_of(")")){
		string expr= str.substr(str.find("(")+1,str.find_last_of(")")-str.find("(")-1);
		return expression(expr, outfile);
	}
	else{   // or returns only the given token

		string token=str;
		                        //checks if there is only one token
		if(tokenCheck(token)){
			//checks if it is a number or it is valid variable name
			if(token.find_first_not_of("0123456789")==string::npos || variablecheck(token)){
				return token;
			}

		}
		error=true;
		return "error";
	}
}
//a simple function that adds allocation lines to the beginning of the toOutfile string
//stores 0 as default
void allocaAndStore(string variable){
	toOutfile = "%" + variable +" = alloca i32\nstore i32 0, i32* %"+ variable+ "\n" + toOutfile;
}
//just a simple transition between sign and corresponding calculation
string op(string token){
	if(token=="+"){
		return "add";
	}else if(token== "-"){
		return "sub";
	}else if(token == "*"){
		return "mul";
	}else{
		return "sdiv";
	}
}
//writes necessary calculations to outfile and returns the result of the postfix using a stack
string evaluate(string postfix, ofstream& outfile){
		if(error){            // optimization:if an error is already detected it returns "error"
			return  "error";
		}
		//the stack used for postfix calculation
		stack<string> calculation;
		istringstream iss(postfix);
		string token;
		while(getline(iss, token, ' ')){  //we tokenize the line
			string tokenT= token;
			//if it is not a sign, we need to load it to a temp ,then push it to the stack
			if(token.find_first_not_of("+-/*")!=string::npos){
				if(declaredVariables.find(token)==declaredVariables.end()&& variablecheck(token)){
					declaredVariables.insert(token);
					allocaAndStore(token) ;
				}
				//if it is not a number or a %t (coming from choose function as a result ) load it to a %t_
				if(variablecheck(token) ){
					tokenT= "%t_"+ to_string(numOfTempVariables);
					toOutfile += tokenT + " = load i32* %" + token + "\n";
					numOfTempVariables++;

				}
				calculation.push(tokenT);
			//do the calculation according to priorities of the signs via push pop operations
			}else{
				string first=calculation.top();
				calculation.pop();
				string second= calculation.top();
				calculation.pop();
				string result="%t_"+to_string(numOfTempVariables);
				numOfTempVariables++;
				toOutfile += result + " = "+ op(token) + " i32 " + second + ", "+ first + "\n";
				calculation.push(result);

			}
		}     // returns the result of the postfix as a string
		return calculation.top();

}
//checks whether there is only one token or more
bool tokenCheck(string& token){
	if(token.empty()){
		return false;
	}
	token= deletedoublespace(token);// deletes blank spaces at the and and the beginning and double blank spaces

	if(token.find(" ")!=string::npos){  // if there is still a blank space we can tell that there is more than one token

		return false;
	}
	return true;

}
// Returns false if expression is built wrong.
bool assignment(ofstream& outfile,string line){

		string left= line.substr(0,line.find("="));

		if(!tokenCheck(left)){  //checks whether the left expression is only one token
			toOutfile="";
			error=true;
			return false;
		}

		//if it is not declared before we add it to our declared variables set
		if(declaredVariables.find(left)== declaredVariables.end()){
			if(variablecheck(left)){
				declaredVariables.insert(left);
				allocaAndStore(left) ;

			}else{
				error=true;
				return false;
			}
		}
		//the part of the line that is after the =
		string right= line.substr(line.find("=")+1);

		string postfix= expression(right, outfile);

		if(error){	//if postfix of the expression has an error, this assignment has a syntax error

			return false;
		}

		//we do the evaluation of the postfix expression by calling evaluate function
		string result=evaluate(postfix, outfile);
		toOutfile += "store i32 " + result + ", i32* %" + left + "\n";
		return true;

}
//checks whether the print expression is valid ,returns false if there are any syntax error
bool print(string line, ofstream& outfile){
	deletedoublespace(line);
	// checks the parenthesis
	if(line.find("(")!=string::npos && line.find_last_of(")")!= string::npos &&line.length()==line.find_last_of(")")+1 && line.find("(")<line.find_last_of(")")){

		string pri= line.substr(0, line.find("("));
		if(tokenCheck(pri)&& pri =="print"){
			int fisrtIndex=line.find("(")+1;
			int lastIndex= line.find_last_of(")");
			string expr= line.substr(fisrtIndex, lastIndex-fisrtIndex); // exp is inside of the parenthesis
			string result= expression(expr, outfile);  // calls expression to create a postfix expression of expr
			if(error){
				return false;
			}
			result= evaluate(result, outfile);			 // finds the result of postfix expression
			toOutfile += "call i32 (i8*, ...)* @printf(i8* getelementptr ([4 x i8]* @print.str, i32 0, i32 0), i32 "+result + " )" +"\n";
			return true;

		}
	}
	error=true;
	return false;
}
//Returns false if if while statements are not syntactically correct.
bool ifWhile(ifstream& infile, ofstream& outfile,string line){

	toOutfile +="br label %wcond" + to_string(whileCount) +"\n";

	if(line.find("(")!=string::npos &&line.find("{")!=string::npos && line.find(")")!= string::npos&& line.find("(")<line.find(")")){
		string whi= line.substr(0, line.find("("));
		// tokenCheck makes sure if there is anything between start and ( other than while or if
		//eliminated errors: whilea(), while a(), awhile
		if(tokenCheck(whi)&& (whi=="while"|| whi== "if")){
			int fisrtIndex=line.find("(")+1;
			int lastIndex= line.find_last_of(")");
			int curly= line.find("{");
			//this checks whether anything written after curly bracket or between ) and {

			if(!(((line.at(lastIndex+1)==' '&&curly== lastIndex+2) || curly== lastIndex+1)&& curly == line.length()-1)){
				error=1;               // checks spaces between the { and ) and the space after }
				return false;

			}					// expr=expression inside the while
			string expr= line.substr(fisrtIndex, lastIndex-fisrtIndex);
			expr= deletedoublespace(expr);
			if(expr.empty()){
				error=1;
				return false;
			}

			 // evaluates the inside of the while condition
			toOutfile += "wcond" + to_string(whileCount) +":" + "\n";
			expr= expression(expr, outfile);
			string result= evaluate(expr, outfile);
			if(error){
				return false;
			}         // adds neccessary llvm codes to the output string
			string temp= "%t_" + to_string(numOfTempVariables);
			numOfTempVariables++;
			toOutfile += temp + " = icmp ne i32 " + result + ", 0" +"\n";
			toOutfile += "br i1 "+ temp + ", label %wbody"+ to_string(whileCount)+ ", label %wend"+ to_string(whileCount)+ "\n";
			toOutfile += "\n";

			toOutfile += "wbody" + to_string(whileCount) + ":" + "\n";
			string whiLines;  // examines the lines inside of curly brackets
			while(getline(infile,whiLines)&& deletedoublespace(whiLines).find("}")==string::npos){
				lineNum++;
				whiLines= deletedoublespace(whiLines);
				
				if(whiLines.empty()){
					
				}else if(whiLines.find("print") != string::npos){

					print(whiLines, outfile);

				}else if(whiLines.find("=")!= string::npos){
					assignment(outfile, whiLines);

				}else{
					error= true;
					return false;
				}
				if(error){
					return false;
				}

			}
	  //whiLines is now  }   OR     there is no }  and the last line of the input file
			whiLines= deletedoublespace(whiLines);  // last line read
				if( !whiLines.empty() && whiLines=="}"){   //  if we reach to }

					if(whi=="while"){
						toOutfile += "br label %wcond" + to_string(whileCount) +"\n";
					}else{
						toOutfile += "br label %wend" + to_string(whileCount) + "\n";
					}
				// if the last line is not only } OR there is no } in the input file

				}else{
					if(whiLines.find("}")!=string::npos){
						lineNum++;
					}

					
					error=true;              
					return false;

				}
                //because we read the line containing } we can continue
				lineNum++;
				toOutfile += "\n";
				toOutfile += "wend" + to_string(whileCount)+":" + "\n";
				whileCount++;
				toOutfile += "\n";


		}else{
			error=1;
			return false;
		}
	}else{   //Syntax error
		toOutfile="";
		error=1;
		return false;
	}
	return true;
}

int main(int argc, char* argv[]){

	ifstream infile;
	
	string a =argv[1];

	infile.open(a);

	string b= a.substr(0, a.find(".my"))+ ".ll";

	ofstream outfile;
	outfile.open(b);
	//writes out the common part into the output file regardless of error
	outfile << "; ModuleID = 'mylang2ir'\n" ;
	outfile <<  "declare i32 @printf(i8*, ...)\n";
	outfile << "@print.str = constant [4 x i8] c\"%d\\0A\\00\"\n";
	outfile<< "@printerror.str = constant [23 x i8] c\"Line %d: syntax error\\0A\\00\"\n";
	choose(outfile);
	outfile <<"define i32 @main() {\n";
	string line;

	bool shouldPrint=true;
	//reading source file line by line
	while(getline(infile, line)){

		line= deletedoublespace(line); // arranges the line

		if(line.find("=")!=string::npos){ // if line includes = calls  the assignment function

			assignment(outfile, line);
		                        // if there is a if or while expression in the line calls ifWhile function
		}else if(line.find("while") != string::npos || line.find("if")!=string::npos){
			ifWhile(infile, outfile,line);

		}else if(line.find("print")!= string::npos){   // if there is a print expression in the line calls print

			print(line, outfile);

		}
		else if(line.empty()||line==" "){    // if line is empty there is nothing to do with the line and it is not an error

		}else{
			error= true;  // if line does not contains "=" or "print" or "if" or "while"

		}
		// if an error has occured after reading the line , we print error with the line number and stop reading the following lines
		if(error){  printError(outfile);
			break;
		}
		lineNum++;  // we increment line number after reading each line

	}
	if(!error){  // after reading the input file, if there is not an error we print the llvm code to output file
		outfile << toOutfile << endl;

	}
	outfile << "ret i32 0" << endl;  // we print common llvm code regardless of the error at the end
	outfile << "}" << endl;
	infile.close();                // we are done
	outfile.close();
	return 0;
}
