#include "utils_sql.h"
#include "utils.h"
#include "db/sqlresults.h"
#include "parser/token.h"
#include "parser/lexer.h"
#include "parser/keywords.h"
#include <QHash>
#include <QPair>
#include <QString>
#include <QDebug>
#include <QMetaType>

QString invalidIdCharacters = "[]()$\"'@*.,+-=/%&|:; \t\n";
QHash<NameWrapper,QPair<QChar,QChar> > wrapperChars;
QList<NameWrapper> sqlite3Wrappers;
QList<NameWrapper> sqlite2Wrappers;

void initUtilsSql()
{
    wrapperChars[NameWrapper::BRACKET] = QPair<QChar,QChar>('[', ']');
    wrapperChars[NameWrapper::QUOTE] = QPair<QChar,QChar>('\'', '\'');
    wrapperChars[NameWrapper::BACK_QUOTE] = QPair<QChar,QChar>('`', '`');
    wrapperChars[NameWrapper::DOUBLE_QUOTE] = QPair<QChar,QChar>('"', '"');

    sqlite3Wrappers << NameWrapper::BRACKET
                    << NameWrapper::QUOTE
                    << NameWrapper::BACK_QUOTE
                    << NameWrapper::DOUBLE_QUOTE;
    sqlite2Wrappers << NameWrapper::BRACKET
                    << NameWrapper::QUOTE
                    << NameWrapper::DOUBLE_QUOTE;

    qRegisterMetaType<SqlResultsPtr>("SqlResultsPtr");
}

bool doesObjectNeedWrapping(const QString& str, Dialect dialect)
{
    if (str.isEmpty())
        return true;

    if (isObjWrapped(str, dialect))
        return false;

    if (isKeyword(str, dialect))
        return true;

    for (int i = 0; i < str.size(); i++)
        if (doesObjectNeedWrapping(str[i]))
            return true;

    return false;
}

bool doesObjectNeedWrapping(const QChar& c)
{
    return invalidIdCharacters.indexOf(c) >= 0;
}

bool isObjectWrapped(const QChar& c, Dialect dialect)
{
    return !doesObjectNeedWrapping(c, dialect);
}

bool isObjectWrapped(const QChar& c)
{
    return !doesObjectNeedWrapping(c);
}

//QString& wrapObjIfNeeded(QString& obj, Dialect dialect, NameWrapper favWrapper)
//{
//    if (doesObjectNeedWrapping(obj, dialect))
//        return wrapObjName(obj, dialect, favWrapper);

//    return obj;
//}

QString wrapObjIfNeeded(const QString& obj, Dialect dialect, NameWrapper favWrapper)
{
    if (doesObjectNeedWrapping(obj, dialect))
        return wrapObjName(obj, dialect, favWrapper);

    return obj;
}

//QString& wrapObjName(QString& obj, Dialect dialect, NameWrapper favWrapper)
//{
//    if (obj.isNull())
//        obj = "";

//    QPair<QChar,QChar> wrapChars = getQuoteCharacter(obj, dialect, favWrapper);

//    if (wrapChars.first.isNull() || wrapChars.second.isNull())
//    {
//        qDebug() << "No quote character possible for object name: " << obj;
//        return obj;
//    }
//    obj.prepend(wrapChars.first);
//    obj.append(wrapChars.second);
//    return obj;
//}

QString wrapObjName(const QString& obj, Dialect dialect, NameWrapper favWrapper)
{
    QString result =  obj;
    if (result.isNull())
        result = "";

    QPair<QChar,QChar> wrapChars = getQuoteCharacter(result, dialect, favWrapper);

    if (wrapChars.first.isNull() || wrapChars.second.isNull())
    {
        qDebug() << "No quote character possible for object name: " << result;
        return result;
    }
    result.prepend(wrapChars.first);
    result.append(wrapChars.second);
    return result;
}

QPair<QChar,QChar> getQuoteCharacter(QString& obj, Dialect dialect, NameWrapper favWrapper)
{
    QList<NameWrapper> wrappers = (dialect == Dialect::Sqlite3) ? sqlite3Wrappers : sqlite2Wrappers;

    // Move favourite wrapper to front of list
    if (wrappers.contains(favWrapper))
    {
        wrappers.removeOne(favWrapper);
        wrappers.insert(0, favWrapper);
    }

    QPair<QChar,QChar> wrapChars;
    foreach (NameWrapper wrapper, wrappers)
    {
        wrapChars = wrapperChars[wrapper];
        if (obj.indexOf(wrapChars.first) > -1)
            continue;

        if (obj.indexOf(wrapChars.second) > -1)
            continue;

        return wrapChars;
    }

    return QPair<QChar,QChar>();
}

QList<QString> wrapObjNames(const QList<QString>& objList, Dialect dialect)
{
    QList<QString> results;
    for (int i = 0; i < objList.size(); i++)
        results << wrapObjName(objList[i], dialect);

    return results;
}

//QString& wrapString(QString& str)
//{
//    str.prepend("'");
//    str.append("'");
//    return str;
//}

QString wrapString(const QString& str)
{
    QString result = str;
    result.prepend("'");
    result.append("'");
    return result;
}

bool doesStringNeedWrapping(const QString& str)
{
    return str[0] == '\'' && str[str.length()-1] == '\'';
}

bool isStringWrapped(const QString& str)
{
    return !doesStringNeedWrapping(str);
}

//QString& wrapStringIfNeeded(QString& str)
//{
//    if (isStringWrapped(str))
//        return wrapString(str);

//    return str;
//}

QString wrapStringIfNeeded(const QString& str)
{
    if (isStringWrapped(str))
        return wrapString(str);

    return str;
}

QString escapeString(QString& str)
{
    return str.replace('\'', "''");
}

QString escapeString(const QString& str)
{
    QString newStr = str;
    return newStr.replace('\'', "''");
}

QString stripString(QString& str)
{
    if (str.length() <= 1)
        return str;

    if (str[0] == '\'' && str[str.length()-1] == '\'')
        return str.mid(1, str.length()-2);

    return str;
}

QString stripString(const QString& str)
{
    QString newStr = str;
    return stripString(newStr);
}

QString stripEndingSemicolon(const QString& str)
{
    QString newStr = rStrip(str);
    if (newStr.size() == 0)
        return str;

    if (newStr[newStr.size()-1] == ';')
    {
        newStr.chop(1);
        return newStr;
    }
    else
        return str;
}

QString stripObjName(const QString &str, Dialect dialect)
{
    QString newStr = str;
    return stripObjName(newStr, dialect);
}

QString stripObjName(QString &str, Dialect dialect)
{
    if (str.isNull())
        return str;

    if (str.length() <= 1)
        return str;

    if (!isObjWrapped(str, dialect))
        return str;

    return str.mid(1, str.length()-2);
}

bool isObjWrapped(const QString& str, Dialect dialect)
{
    return getObjWrapper(str, dialect) != NameWrapper::null;
}

NameWrapper getObjWrapper(const QString& str, Dialect dialect)
{
    if (str.isEmpty())
        return NameWrapper::null;

    QList<NameWrapper> wrappers;

    if (dialect == Dialect::Sqlite2)
        wrappers = sqlite2Wrappers;
    else
        wrappers = sqlite3Wrappers;

    foreach (NameWrapper wrapper, wrappers)
    {
        QPair<QChar,QChar> chars = wrapperChars[wrapper];
        if (str[0] == chars.first && str[str.length()-1] == chars.second)
            return wrapper;
    }
    return NameWrapper::null;
}

bool isWrapperChar(const QChar& c, Dialect dialect)
{
    QList<NameWrapper> wrappers;
    if (dialect == Dialect::Sqlite2)
        wrappers = sqlite2Wrappers;
    else
        wrappers = sqlite3Wrappers;

    foreach (NameWrapper wrapper, wrappers)
    {
        QPair<QChar,QChar> chars = wrapperChars[wrapper];
        if (c == chars.first || c == chars.second)
            return true;
    }
    return false;
}

int qHash(NameWrapper wrapper)
{
    return (uint)wrapper;
}

QString getPrefixDb(const QString& origDbName, Dialect dialect)
{
    if (origDbName.isEmpty())
        return "main";
    else
        return wrapObjIfNeeded(origDbName, dialect);
}

bool isSystemTable(const QString &name)
{
    return name.startsWith("sqlite_");
}

bool isSystemIndex(const QString &name, Dialect dialect)
{
    switch (dialect)
    {
        case Dialect::Sqlite3:
            return name.startsWith("sqlite_autoindex_");
        case Dialect::Sqlite2:
        {
            QRegExp re("*(*autoindex*)*");
            re.setPatternSyntax(QRegExp::Wildcard);
            return re.exactMatch(name);
        }
    }
    return false;
}


TokenPtr stripObjName(TokenPtr token, Dialect dialect)
{
    if (!token)
        return token;

    token->value = stripObjName(token->value, dialect);
    return token;
}

QString removeComments(const QString& value)
{
    Lexer lexer(Dialect::Sqlite3);
    TokenList tokens = lexer.tokenize(value);
    while (tokens.remove(Token::COMMENT))
        continue;

    return tokens.detokenize();
}

QStringList splitQueries(const QString& sql, Dialect dialect)
{
    TokenList tokens = Lexer::tokenize(sql, dialect);

    QStringList queries;
    TokenList currentQueryTokens;
    QString value;
    int createTriggerMeter = 0;
    bool insideTrigger = false;
    foreach (const TokenPtr& token, tokens)
    {
        value = token->value.toUpper();
        if (insideTrigger)
        {
            if (token->type == Token::KEYWORD && value == "END")
                insideTrigger = false;

            currentQueryTokens << token;
            continue;
        }

        if (token->type == Token::KEYWORD)
        {
            if (value == "CREATE" || value == "TRIGGER" || value == "BEGIN")
                createTriggerMeter++;

            if (createTriggerMeter == 3)
                insideTrigger = true;

            currentQueryTokens << token;
        }
        else if (token->type == Token::OPERATOR && value == ";")
        {
            createTriggerMeter = 0;
            currentQueryTokens << token;
            queries << currentQueryTokens.detokenize();
            currentQueryTokens.clear();
        }
        else
        {
            currentQueryTokens << token;
        }
    }

    if (currentQueryTokens.size() > 0)
        queries << currentQueryTokens.detokenize();

    return queries;
}

QString getQueryWithPosition(const QStringList& queries, int position, int* startPos)
{
    int currentPos = 0;
    int length = 0;

    if (startPos)
        *startPos = 0;

    foreach (const QString& query, queries)
    {
        length = query.length();
        if (position >= currentPos && position < currentPos+length)
            return query;

        currentPos += length;

        if (startPos)
            *startPos += length;
    }

    // If we passed all queries and it happens that the cursor is just after last query - this is the query we want.
    if (position == currentPos && queries.size() > 0)
    {
        if (startPos)
            *startPos -= length;

        return queries.last();
    }

    if (startPos)
        *startPos = -1;

    return QString::null;
}

QString getQueryWithPosition(const QString& queries, int position, Dialect dialect, int* startPos)
{
    QStringList queryList = splitQueries(queries, dialect);
    return getQueryWithPosition(queryList, position, startPos);
}
