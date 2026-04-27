%{
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "ast.h"
#include "parser.tab.h"
#include "common.h"

Token token_table[MAX_TOKENS];
int   token_count = 0;
int   lex_errors  = 0;

static void record(TokenType tt, const char *val) {
    if (token_count < MAX_TOKENS) {
        token_table[token_count].type = tt;
        strncpy(token_table[token_count].value, val, MAX_LEN - 1);
        token_table[token_count].line = yylineno;
        token_count++;
    }
}
%}

%option noyywrap
%option caseless
%option yylineno
%option nomain


ACTION  study|revise|practice|review|complete|finish|read|solve|attempt|prepare|memorize|memorise|watch|write|understand|analyze|analyse|summarize|summarise|implement|debug|test|code|draw|research|explore|learn|master|attend|plan|schedule|organize|organise|cover|tackle|focus|work|do|start|begin|continue|go|through|brush|up|catch|glance|skim|scan|assess|evaluate|check|verify|run|execute|compile|build|design|create|develop|improve|revamp|rethink|rewrite|redo|repeat|go|through|recall|recite|explain|teach|present|discuss|collaborate|brainstorm|outline|draft|edit|proofread|publish|submit|upload|download|install|configure|deploy|test|benchmark|profile|optimize|refactor|document|comment|review|merge|push|pull|clone|fork|branch|commit|review|iterate|prototype|sketch|model|simulate|validate|experiment|hypothesize|formulate|derive|prove|calculate|compute|solve|evaluate|integrate|differentiate|apply|practice|drill|exercise|attempt|try

SUBJECT compiler|system_software|computer_network|web_development|operating_system|data_structures|algorithms|database|machine_learning|artificial_intelligence|mathematics|discrete_math|software_engineering|computer_architecture|programming|python|java|cpp|c\+\+|csharp|c#|cn|os|ds|ai|ml|dbms|se|physics|chemistry|biology|statistics|networks|security|cloud|devops|flutter|react|reactjs|nodejs|node|sql|nosql|html|css|javascript|typescript|rust|go|golang|kotlin|swift|calculus|linear_algebra|probability|english|economics|finance|management|psychology|history|geography|electronics|signals|microprocessors|vlsi|embedded|cryptography|blockchain|iot|robotics|numerical_methods|theory_of_computation|toc|automata|formal_languages|compiler_design|cd|computer_graphics|cg|digital_logic|dl|computer_vision|cv|natural_language_processing|nlp|deep_learning|dl|reinforcement_learning|rl|data_science|ds|big_data|cloud_computing|parallel_computing|distributed_systems|networking|cyber_security|information_security|ethical_hacking|penetration_testing|quantum_computing|game_development|mobile_development|android|ios|linux|unix|windows|macos|bash|shell|scripting|automation|testing|qa|agile|scrum|project_management|product_management|ui|ux|design|figma|sketch|photoshop|illustrator|after_effects|premiere|matlab|r|scala|perl|ruby|php|asp|django|flask|spring|angular|vue|svelte|nextjs|express|fastapi|tensorflow|pytorch|keras|sklearn|pandas|numpy|scipy|matplotlib|seaborn|tableau|powerbi|excel|word|powerpoint|aws|azure|gcp|docker|kubernetes|jenkins|git|github|gitlab|bitbucket|jira|confluence|slack|notion|linear|trello|asana|airtable|figma|canva|latex|markdown|json|xml|yaml|api|rest|graphql|grpc|microservices|monolith|serverless|event_driven|cqrs|ddd|tdd|bdd|ci_cd|devops|sre|mlops|data_engineering|etl|spark|hadoop|kafka|redis|mongodb|postgresql|mysql|sqlite|oracle|cassandra|elasticsearch|solr|neo4j|dynamodb|firebase|supabase|vercel|netlify|heroku|digital_ocean|linode|vultr

EVENT   lunch|dinner|breakfast|class|exam|lab|meeting|lecture|seminar|quiz|tutorial|workshop|break|gym|office_hours|hackathon|internship|presentation|project|assignment|deadline|work|college|school|practice|session|call|standup|sprint|review|retrospective|planning|interview|test|exercise|yoga|meditation|prayer|church|mosque|temple|party|event|competition|contest|olympiad|conference|webinar|podcast|stream|video|movie|show|game|match|training|coaching|mentoring|study_group|group_study|pair_programming|coding_session|reading_session|writing_session|research_session|lab_session|clinic|appointment|checkup|therapy|counseling|tutoring|tutoring_session|office|work_session|freelance|side_project|personal_project|date|outing|hangout|trip|travel|commute|drive|walk|run|jog|swim|sport|sports|nap|sleep|rest|chores|errands|shopping|cooking|eating|prayer|pooja

TIMEOF  morning|afternoon|evening|night|midnight|dawn|dusk|noon|late_night|early_morning|late_evening|mid_morning|mid_afternoon|daytime|nighttime|bedtime|wakeup|sunrise|sunset

DAY     monday|tuesday|wednesday|thursday|friday|saturday|sunday|weekday|weekend|today|tomorrow|yesterday|everyday|daily|weekly|mon|tue|wed|thu|fri|sat|sun

PREP_FOR    for
PREP_TIME   after|before|during|at|by|around|near|following|preceding
PREP_OTHER  until|from|to|in|on|within|throughout|across|over|this|next|every|each

CONNECTOR   and|then|also|next|plus|additionally|furthermore|moreover|after_that|afterwards|subsequently|followed_by|then_also


TIMEUNIT    hours|hour|minutes|minute|mins|hrs|min|hr|h|m|seconds|second|secs|sec|days|day|weeks|week


MODIFIER    quick|deep|light|intensive|brief|thorough|focused|active|passive|advanced|basic|detailed|comprehensive|rapid|slow|careful|efficient|effective|productive|aggressive|lazy|casual|serious|strict|regular|irregular|consistent|dedicated|committed|motivated|disciplined|flexible|structured|unstructured|formal|informal|solo|group|online|offline|hybrid|interactive|hands_on|theoretical|practical|applied|conceptual|analytical|creative|critical|strategic|tactical|methodical|systematic|holistic|incremental|iterative

PRIORITY    high|medium|low|critical|urgent|optional|important|normal|minor|major|essential|needed|required|mandatory|nice_to_have|bonus|extra


IGNORE      i|want|to|wanna|gonna|need|have|should|must|will|would|could|the|a|an|my|some|any|this|that|it|is|are|was|were|be|been|being|get|got|getting|go|going|make|making|take|taking|do|doing|can|may|might|shall|had|has|having|let|lets|please|ok|okay|sure|yeah|yep|yes|no|not|so|very|really|quite|just|only|even|still|yet|already|now|soon|later|never|always|sometimes|often|rarely|usually|generally|specifically|especially|particularly|mainly|mostly|totally|completely|absolutely|definitely|probably|possibly|maybe|perhaps|actually|basically|literally|honestly|seriously|hopefully|fortunately|unfortunately|meanwhile|however|therefore|thus|hence|consequently|accordingly|otherwise|instead|rather|despite|although|though|while|since|because|as|if|whether|when|where|who|what|which|whom|whose|how|why|there|here|too|also|both|either|neither|each|every|all|many|much|more|most|few|less|least|several|various|different|same|other|another|else|such|than|like|unlike|about|above|below|between|among|within|without|along|across|around|through|throughout|against|towards|toward|onto|into|out|up|down|off|away|back|forward|together|apart|aside|along|ahead|behind|around|nearby|far|close|next|last|first|second|third|final|previous|following|upcoming|current|present|past|future|recent|new|old|young|big|small|large|short|long|fast|slow|easy|hard|simple|complex|clear|unclear|good|bad|great|poor|excellent|terrible|perfect|complete|incomplete|full|empty|open|closed

NUMBER  [0-9]+(\.[0-9]+)?
WS      [ \t\r\n]+

%%

{IGNORE}    

{ACTION}    {
                strncpy(yylval.sval, yytext, 127);
                record(TT_ACTION, yytext);
                return TOK_ACTION;
            }

{MODIFIER}  {
                strncpy(yylval.sval, yytext, 127);
                record(TT_MODIFIER, yytext);
                return TOK_MODIFIER;
            }

{PRIORITY}  {
                strncpy(yylval.sval, yytext, 127);
                record(TT_PRIORITY, yytext);
                return TOK_PRIORITY;
            }

{SUBJECT}   {
                strncpy(yylval.sval, yytext, 127);
                record(TT_SUBJECT, yytext);
                return TOK_SUBJECT;
            }

{EVENT}     {
                strncpy(yylval.sval, yytext, 127);
                record(TT_EVENT, yytext);
                return TOK_EVENT;
            }

{TIMEOF}    {
                strncpy(yylval.sval, yytext, 127);
                record(TT_TIMEOF, yytext);
                return TOK_TIMEOF;
            }

{DAY}       {
               strncpy(yylval.sval, yytext, 127);
               record(TT_TIMEOF, yytext);
               return TOK_TIMEOF;
}

{PREP_FOR}  {
                strncpy(yylval.sval, yytext, 127);
                record(TT_PREPOSITION, yytext);
                return TOK_FOR;
            }

{PREP_TIME} {
                strncpy(yylval.sval, yytext, 127);
                record(TT_PREPOSITION, yytext);
                return TOK_PREP_TIME;
            }

{PREP_OTHER} {
                strncpy(yylval.sval, yytext, 127);
                record(TT_PREPOSITION, yytext);
                return TOK_PREP_OTHER;
            }

{CONNECTOR} {
                strncpy(yylval.sval, yytext, 127);
                record(TT_CONNECTOR, yytext);
                return TOK_CONNECTOR;
            }

{TIMEUNIT}  {
                strncpy(yylval.sval, yytext, 127);
                record(TT_TIMEUNIT, yytext);
                return TOK_TIMEUNIT;
            }

{NUMBER}    {
                yylval.ival = atoi(yytext);
                record(TT_NUMBER, yytext);
                return TOK_NUMBER;
            }

{WS}        { /* skip whitespace */ }

[a-zA-Z_][a-zA-Z0-9_]* {
                /* Unknown word — report a LEXICAL ERROR */
                lex_error_add(yylineno, yytext);
                lex_errors++;
            }

. {
                char buf[32];
                snprintf(buf, sizeof(buf), "'%s'", yytext);
                lex_error_add(yylineno, buf);
                lex_errors++;
            }

%%
