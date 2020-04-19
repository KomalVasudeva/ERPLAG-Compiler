#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "typeChecker.h"
#include "codegen.h"

extern baseST *realBase ;
extern int x ;

int moduleGeneration (astNode *node, int localBase, int rspDepth, moduleST *lst, varST *vst, FILE *fp)
{
	if (node == NULL)
		return 0 ;

	switch (node->id)
	{
		int start_label, end_label ;
		int reserveLabel[10] ;
		varST *searchedVar ;

		case statements :
			moduleGeneration (node->child, localBase, rspDepth, node->localST, vst, fp);		// Access local scope and move below
			break ;

		case statement :
			rspDepth = moduleGeneration(node->child, localBase, rspDepth, lst, vst, fp);
			if (node->next == NULL)
			{
				if (lst->parent == realBase)
				{
					fprintf (fp, "\n\tMOV RSP, RBP\n") ;
					fprintf (fp, "\tPOP RBP\n") ;
					fprintf (fp, "\tPOP RBP\n") ;
					fprintf (fp, "\tret\n") ;
				}
				else if (rspDepth - localBase > 0)
					fprintf (fp, "\tADD RSP, %d\t\t\t;Restoring RSP to previous scope\n", (rspDepth-localBase)) ;
			}
			else
				moduleGeneration(node->next, localBase, rspDepth, lst, vst, fp);
			break ;

		case TK_DECLARE :
			;
			int endOffset, declCount = 1 ;
			astNode *idListHead = node->next->child ;
			astNode *dtNode = node->next->next ;

			while (idListHead->next != NULL)
			{
				declCount++ ;
				idListHead = idListHead->next ;
			}

			searchedVar = searchVar (realBase, lst, idListHead->tok->lexeme) ;
			endOffset = searchedVar->offset ;
			if (endOffset > 0)
			{		
				fprintf (fp, "\tSUB RSP, %d\t\t\t;Updating RSP\n\n", (endOffset - rspDepth)) ;
				rspDepth = endOffset ;
			}

			if (dtNode->dtTag == ARRAY && !isVarStaticArr(searchedVar))	
			{
				dynamicDeclareCheck (lst, searchedVar, fp) ;

				if (declCount > 1)
				{
					start_label = get_label () ;
					fprintf (fp, "\n\tMOV CX, 0\n") ;
					fprintf (fp, "\tMOV RBX, %d\n", searchedVar->offset) ;
					fprintf (fp, "\tNEG RBX\n") ;
					fprintf (fp, "\nLABEL%d:\n", start_label) ;

					fprintf (fp, "\tPUSH CX\n") ;
					fprintf (fp, "\tPUSH RBX\n") ;
					fprintf (fp, "\tPUSH RDI\n") ;
					fprintf (fp, "\tCALL malloc\n") ;
					fprintf (fp, "\tPOP RDI\n") ;
					fprintf (fp, "\tPOP RBX\n") ;
					fprintf (fp, "\tPOP CX\n") ;
					fprintf (fp, "\tMOV [RBP+RBX], RAX\n") ;

					fprintf (fp, "\tPOP AX\n") ;
					fprintf (fp, "\tMOV [RBP + RBX + 10], AX\n") ;
					fprintf (fp, "\tPOP DX\n") ;
					fprintf (fp, "\tMOV [RBP + RBX + 8], DX\n") ;

					fprintf (fp, "\tPUSH DX\n") ;
					fprintf (fp, "\tPUSH AX\n") ;
					fprintf (fp, "\tADD RBX, 12\n") ;
					fprintf (fp, "\tINC CX\n") ;
					fprintf (fp, "\tCMP CX, %d\n", declCount) ;
					fprintf (fp, "\tJL LABEL%d\n", start_label) ;

					fprintf (fp, "\tADD RSP, 4\n");
				}
				else
				{
					fprintf (fp, "\n\tCALL malloc\n") ;
					fprintf (fp, "\tMOV [RBP-%d], RAX\n", searchedVar->offset) ;
					fprintf (fp, "\tPOP AX\n") ;
					fprintf (fp, "\tMOV [RBP-%d], AX\n", searchedVar->offset - 10) ;
					fprintf (fp, "\tPOP BX\n") ;
					fprintf (fp, "\tMOV [RBP-%d], BX\n", searchedVar->offset - 8) ;
				}
			}

			
			break ;											

		case TK_ASSIGNOP :
			moduleGeneration(node->child->next, localBase, rspDepth,  lst, vst, fp);
			moduleGeneration(node->child, localBase, rspDepth, lst, vst, fp);
			fprintf (fp, "\n") ;
			break ;

		case TK_PLUS :
			moduleGeneration(node->child, localBase, rspDepth, lst, vst, fp);
			moduleGeneration(node->child->next, localBase, rspDepth,  lst, vst, fp);
			fprintf (fp, "\tPOP AX\n");
			fprintf (fp, "\tPOP BX\n");
			fprintf (fp, "\tADD AX,BX\n");
			fprintf (fp, "\tPUSH AX\n");

			break ;

		case TK_MINUS :
			moduleGeneration(node->child->next, localBase, rspDepth, lst, vst, fp);
			moduleGeneration(node->child, localBase, rspDepth, lst, vst, fp);
			fprintf (fp, "\tPOP AX\n");
			fprintf (fp, "\tPOP BX\n");
			fprintf (fp, "\tSUB AX,BX\n");
			fprintf (fp, "\tPUSH AX\n");

			break ;

		case TK_MUL :
			moduleGeneration(node->child->next, localBase, rspDepth, lst, vst, fp);
			moduleGeneration(node->child, localBase, rspDepth, lst, vst, fp);
			fprintf (fp, "\tPOP AX\n");
			fprintf (fp, "\tPOP BX\n");
			fprintf (fp, "\tIMUL BX\n");
			fprintf (fp, "\tPUSH AX\n");

			break ;

		case TK_DIV :
			moduleGeneration(node->child, localBase, rspDepth, lst, vst, fp);
			moduleGeneration(node->child->next, localBase, rspDepth, lst, vst, fp);
			fprintf (fp, "\tPOP BX\n");
			fprintf (fp, "\tPOP AX\n\n");

			fprintf (fp, "\tIDIV BX\n");
			fprintf (fp, "\tPUSH AX\n");
			break ;

		case TK_RNUM :
			fprintf (fp, "NO RNUM ALLOWED!\n") ;
			exit (0) ;
			break ;

		case TK_NUM : 
			if (node->prev == NULL)
			{
				fprintf (fp, "\tMOV AX, %s\n",node->tok->lexeme);
				fprintf (fp, "\tPUSH AX\n");	
			}
			else
			{
				fprintf (fp, "\tMOV BX, %s\n",node->tok->lexeme);
				fprintf (fp, "\tPUSH BX\n");	
			}
			break ;

		case TK_TRUE :
			if (node->prev == NULL)
			{
				fprintf (fp, "\tMOV AX,00000001H\n");
				fprintf (fp, "\tPUSH AX\n");	
			}
			else
			{
				fprintf (fp, "\tMOV BX,00000001H\n");
				fprintf (fp, "\tPUSH BX\n");	
			}
			break ;

		case TK_FALSE :
			if (node->prev == NULL)
			{
				fprintf (fp, "\tMOV AX,00000000H\n");
				fprintf (fp, "\tPUSH AX\n");	
			}
			else
			{
				fprintf (fp, "\tMOV BX,00000000H\n");
				fprintf (fp, "\tPUSH BX\n");	
			}
			break ;


		case TK_ID :
			IDGeneration (node, lst, vst, fp) ;
			break ;
		
		case TK_FOR :
			node=node->next;

			fprintf (fp, "\tMOV CX,%s\n", node->next->tok->lexeme);
			fprintf (fp, "\tMOV [RBP - %d], CX\t\t;for loop lower lim\n" , searchVar(realBase, lst, node->tok->lexeme)->offset);

			start_label = get_label();
			end_label = get_label();
			fprintf (fp, "\nLABEL%d:\n",start_label);

			fprintf (fp, "\tMOV AX, %s\n", node->next->next->tok->lexeme) ;
			fprintf (fp, "\tCMP CX,AX\n");
			fprintf (fp, "\tJG LABEL%d\n",end_label);

			moduleGeneration(node->next->next->next, rspDepth, rspDepth, lst, vst, fp);		// Statements

			fprintf (fp, "\n\tMOV CX, [RBP - %d]\t\t;Ending increment\n", searchVar(realBase, lst, node->tok->lexeme)->offset);
			fprintf (fp, "\tINC CX\n");
			fprintf (fp, "\tMOV [RBP - %d],CX\n", searchVar(realBase, lst, node->tok->lexeme)->offset);
			fprintf (fp, "\tJMP LABEL%d",start_label);
			fprintf (fp, "\n\nLABEL%d:\n",end_label);
			break ;

		
		case TK_WHILE :
			node=node->next;
			start_label = get_label();
			end_label =  get_label();
			fprintf (fp, "\t\nLABEL%d:\n",start_label);

			moduleGeneration(node, localBase, rspDepth, lst, vst, fp);	// expression

			fprintf (fp, "\tPOP AX\n");
			fprintf (fp, "\tCMP AX,00000001h\n");
			fprintf (fp, "\tJNE LABEL%d\n",end_label);

			moduleGeneration(node->next, rspDepth, rspDepth, lst, vst, fp);		// statements

			fprintf (fp, "\tJMP LABEL%d\n",start_label);
			fprintf (fp, "\nLABEL%d:\n",end_label);

			break ;

		// else if(node->id == TK_LT || node->id == TK_GT || node->id == TK_LE ||node->id == TK_GE ||node->id == TK_NE ||node->id == TK_EQ)  //all correct
		case TK_LT : case TK_GT : case TK_LE : case TK_GE : case TK_NE : case TK_EQ :
			moduleGeneration(node->child, localBase, rspDepth, lst, vst, fp);
			moduleGeneration(node->child->next, localBase, rspDepth, lst, vst, fp);

			fprintf (fp, "\tPOP BX\n");
			fprintf (fp, "\tPOP AX\n");

			start_label=get_label();
			end_label=get_label();
			fprintf (fp, "\tCMP AX,BX\n\n");

			switch (node->id)
			{
				case TK_LT :
					fprintf (fp, "\tJL LABEL%d\n",start_label);
					break ;
				case TK_GT :
					fprintf (fp, "\tJG LABEL%d\n",start_label);
					break ;
				case TK_LE :
					fprintf (fp, "\tJLE LABEL%d\n",start_label);
					break ;
				case TK_GE :
					fprintf (fp, "\tJGE LABEL%d\n",start_label);
					break ;
				case TK_NE :
					fprintf (fp, "\tJNE LABEL%d\n",start_label);
					break ;
				case TK_EQ :
					fprintf (fp, "\tJE LABEL%d\n",start_label);
					break ;
			}


			fprintf (fp, "\tMOV AX,0\n");
			fprintf (fp, "\tPUSH AX\n");
			fprintf (fp, "\tJMP LABEL%d\n\n",end_label);
			fprintf (fp, "LABEL%d:\n\n",start_label);
			fprintf (fp, "\tMOV AX,1\n");
			fprintf (fp, "\tPUSH AX\n\n\n");
			fprintf (fp, "LABEL%d:\n\n",end_label);

			break ;

		case TK_AND : case TK_OR :
			moduleGeneration(node->child, localBase, rspDepth, lst, vst, fp);
			moduleGeneration(node->child->next, localBase, rspDepth, lst, vst, fp);

			start_label = get_label();
			int mid_label = get_label();
			end_label = get_label();

			fprintf (fp, "\tPOP BX\n");
			fprintf (fp, "\tPOP AX\n"); 

			fprintf (fp, "\tCMP AX,00000001h\n");
			if (node->id == TK_AND)
			{
				fprintf (fp, "\tJE LABEL%d\n",start_label);
				fprintf (fp, "\tMOV AX,00000000h\n");
			}
			else
			{
				fprintf (fp, "\tJNE LABEL%d\n",start_label);
				fprintf (fp, "\tMOV AX,00000001h\n");
			}

			fprintf (fp, "\tPUSH AX\n");
			fprintf (fp, "\tJMP LABEL%d\n",end_label);


			fprintf (fp, "\t\n\nLABEL%d:\n",start_label);
			fprintf (fp, "\tCMP BX,00000001h\n");
			fprintf (fp, "\tJNE LABEL%d\n",mid_label);
			fprintf (fp, "\tMOV AX,00000001h\n");
			fprintf (fp, "\tPUSH AX\n");
			fprintf (fp, "\tJMP LABEL%d\n",end_label);
			fprintf (fp, "LABEL%d:\n",mid_label);
			fprintf (fp, "\tMOV AX,00000000h\n");
			fprintf (fp, "\tPUSH AX\n");
			fprintf (fp, "LABEL%d:\n",end_label);

			break ;

		case TK_PRINT :
			node = node->next ;
			searchedVar = searchVar (realBase, lst, node->tok->lexeme) ;

			printGeneration (node, lst, searchedVar, fp) ;
			break ;

		case TK_GET_VALUE :
			searchedVar = searchVar (realBase, lst, node->next->tok->lexeme) ;

			getValueGeneration (lst, searchedVar, rspDepth, fp) ;
			break ;

	}

	return rspDepth ;
}

void codeGeneration(astNode *node, FILE* fp)
{
	if (node == NULL)
		return;

	switch (node->id)
	{
		case program :
			codeGeneration (node->child->next, fp) ;		// <otherModules>
			break ;

		case otherModules :
			codeGeneration (node->child, fp) ;					// Do module definitions
			codeGeneration (node->next, fp) ;					// <driverModule> or NULL
			break ;

		case driverModule :
			moduleGeneration(node->child, 0, 0, NULL, NULL, fp); 				// <statements>
			codeGeneration(node->next, fp); 				// Move to the next child of program
			break ;
	}
}

int main(int argc, char *argv[])
{
	if (argc != 2 && argc != 3)
	{
		printf ("\tIncorrect number of arguments\n") ;
		exit (0) ;
	}

	treeNode *root = parseTree (argv[1]) ;
	astNode *astRoot ;
	astRoot = applyASTRule (root) ;
	realBase = fillSymbolTable(astRoot,0);

	if (argc == 3)
	{
		if (strcmp(argv[2], "-printAST") == 0)
			inorderAST (astRoot, 0) ;
		else
		{
			printf ("Incorrect command!\n") ;
			exit (0) ;
		}
	}

	if (!realBase->semanticError)
	{
		FILE *fp = fopen ("code.asm", "w") ;

		preamble (fp) ;
		codeGeneration (astRoot, fp) ;
	}
	else
		printf ("Semantic errors found. Please check the above messages\n") ;
	return 0;
}