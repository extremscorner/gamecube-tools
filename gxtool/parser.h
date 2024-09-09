#ifndef __PARSER_H__
#define __PARSER_H__

#include <filesystem>

class CTokenString;

using namespace std;
using namespace std::filesystem;

class CParser
{
public:
	struct _ttokenstringlist {
	public:
		_ttokenstringlist(const char *pszTokenString,int nLen);
		~_ttokenstringlist();

		const char* GetTokenValue(const char *pszTokenName,const char *pszDefault = NULL) const;

		int nTokens;
		CTokenString *pToken;
		_ttokenstringlist *pNext;
	};

public:
	CParser();
	virtual ~CParser();

	int Parse(int argc,const char **argv);

	void ReleaseAll();

	const char* GetOutputFilename() {return m_sOutFilename.c_str();}
	const char* GetInputFilename() {return m_sInFilename.c_str();}
	const _ttokenstringlist* GetEntries() {return m_pArgList;}
	path GetScriptPath() { return m_sScriptPath; }
	path GetDepsFilename() { return m_sDepFilename; }

protected:
	void Empty();
	int LoadScriptfile(const char *pszFilename);
	int LoadBuffer(char *pBuffer,int nLen);
	int NewTokenString(const char *pszTokenString,int nLen);

protected:
	string m_sOutFilename;
	string m_sInFilename;
	path m_sScriptPath;
	path m_sDepFilename;

	_ttokenstringlist *m_pArgList;
};

#endif
