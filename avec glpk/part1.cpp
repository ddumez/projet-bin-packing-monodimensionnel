/* DUMEZ dorian 
   AUTIN-MARTINEAU kevin*/

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glpk.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <algorithm>
#include <vector>

using namespace std;

/* Structure contenant les données du problème */

typedef struct {
	int t; // Taille de l'objet
	int nb; // Nombre d'objets de la même taille 
} objets;



typedef struct {
	int T; // Taille du bin 
	int nb; // Nombre de tailles différentes pour les objets
	vector <objets *> tab; // Tableau des objets (taille + nombre dans une même taille)
	int nbcreu;
} donnees;
bool compare ( objets *a, objets *b) { return a->t > b->t;}

void trouve_motif ( vector <vector <int> > *motif, donnees *p);
void complete_motif ( vector <int> *base, donnees *p, vector <vector <int> > *motif);

/* lecture des donnees */
void lecture_data(char *file, donnees *p)
{
	
	FILE *fin; // Pointeur sur un fichier
	int i;	
	int val;
	objets *tmp;
	
	fin = fopen(file,"r"); // Ouverture du fichier en lecture
	
	p->nbcreu = 0; //initialisation de nbcreu
	
	/* Première ligne du fichier, on lit la taille du bin, et le nombre de tailles différentes pour les objets à ranger */
	
	fscanf(fin,"%d",&val);
	p->T = val;
	fscanf(fin,"%d",&val);
	p->nb = val;
	
	/* On lit ensuite les infos sur les (taille d'objets + nombres) */
	
	for(i = 0;i < p->nb;i++) // Pour chaque format d'objet...
	{
		tmp = new objets();
		// ... on lit les informations correspondantes
		fscanf(fin,"%d",&val);
		tmp->t = val;
		fscanf(fin,"%d",&val);
		tmp->nb = val;
		
		p->tab.push_back(tmp);
	}
	
	fclose(fin); // Fermeture du fichier
}

/* Quelques fonctions utiles pour la mesure du temps CPU */

struct timeval start_utime, stop_utime;

void crono_start()
{
	struct rusage rusage;
	
	getrusage(RUSAGE_SELF, &rusage);
	start_utime = rusage.ru_utime;
}

void crono_stop()
{
	struct rusage rusage;
	
	getrusage(RUSAGE_SELF, &rusage);
	stop_utime = rusage.ru_utime;
}

double crono_ms()
{
	return (stop_utime.tv_sec - start_utime.tv_sec) * 1000 +
    (stop_utime.tv_usec - start_utime.tv_usec) / 1000 ;
}

/* Fonctions à ajouter
	.
	.
	.
 */
 
int main(int argc, char *argv[])
{	
	/* Données du problème */

	donnees p; 
		
	// autres déclarations 
	vector < vector <int> > motifs; //vectur de l'ensembles des motifs : ligne numero du motif, colonne : nombre d'objet de ce type dans ce motif
	
	/* Chargement des données à partir d'un fichier */
	lecture_data(argv[1],&p);
	
	/* Lancement du chronomètre! */
	crono_start(); 
	
	//trouver tout les bins possibles
	trouve_motif(&motifs, &p);
	
	/*rentrer toutes les données dans le solveur */
	/* structures de données propres à GLPK */
	
	glp_prob *prob; // déclaration d'un pointeur sur le problème
	int *ia; 
	int *ja;
	double *ar; // déclaration des 3 tableaux servant à définir la matrice "creuse" des contraintes
	
	/* Les déclarations suivantes sont optionnelles, leur but est de donner des noms aux variables et aux contraintes.
	   Cela permet de lire plus facilement le modèle saisi si on en demande un affichage à GLPK, ce qui est souvent utile pour détecter une erreur! */
	
	char **nomcontr;
	char **numero;	
	char **nomvar; 

    /* variables récupérant les résultats de la résolution du problème (fonction objectif et valeur des variables) */
	
	double z; 
	double *x; 
	
	/* autres déclarations */ 
	
	int i,j;
	int pos; // compteur utilisé dans le remplissage de la matrice creuse
		
	
	/* Transfert de ces données dans les structures utilisées par la bibliothèque GLPK */
	
	prob = glp_create_prob();
	glp_set_prob_name(prob, "bin-packing");
	glp_set_obj_dir(prob, GLP_MIN); //probleme de minimisation
	
	/* Déclaration du nombre de contraintes (nombre de lignes de la matrice des contraintes) : p.nbcontr */
	
	glp_add_rows(prob, p.nb); //une contrainte par type d'objet 
	nomcontr = (char **) malloc (p.nb * sizeof(char *));
	numero = (char **) malloc (p.nb * sizeof(char *)); 

	//partie droite des contraintes

	for(i=1;i<=p.nb;i++)
	{
		/* partie optionnelle : donner un nom aux contraintes */
		nomcontr[i - 1] = (char *) malloc (8 * sizeof(char)); // hypothèse simplificatrice : on a au plus 7 caractères, sinon il faudrait dimensionner plus large! 
		numero[i - 1] = (char *) malloc (3  * sizeof(char)); // Même hypothèse sur la taille du problème
		strcpy(nomcontr[i-1], "type");
		sprintf(numero[i-1], "%d", i);
		strcat(nomcontr[i-1], numero[i-1]); /* Les contraintes sont nommés "type1", "type2"... */		
		glp_set_row_name(prob, i, nomcontr[i-1]); /* Affectation du nom à la contrainte i */
		
		//la borne en elle même
		glp_set_row_bnds(prob, i, GLP_LO, p.tab.at(i-1)->nb, 0.0); 
		/*contrainte du type >= 
		la partie droite corespond au nombre d'objet de ce type à ranger*/
	}	
	delete(numero);
	//nombre de variable
	
	glp_add_cols(prob, motifs.size() ); 
	nomvar = (char **) malloc ( motifs.size() * sizeof(char *));
	numero = (char **) malloc ( motifs.size() * sizeof(char *)); 
	//type des variables
	
	for(i=1;i<= motifs.size();i++)
	{

		//nomage des variables
		nomvar[i - 1] = (char *) malloc (15 * sizeof(char));
		numero[i - 1] = (char *) malloc (10  * sizeof(char)); // Même hypothèse sur la taille du problème
		strcpy(nomvar[i-1], "motif");
		sprintf(numero[i-1], "%d", i);
		strcat(nomvar[i-1], numero[i-1]);
		glp_set_col_name(prob, i , nomvar[i-1]); //les variables sont nomée motif1, motif2, ...
		
		//borne et type sur les variables
		glp_set_col_bnds(prob, i, GLP_LO, 0.0, 0.0); /* bornes sur les variables, comme sur les contraintes */
		glp_set_col_kind(prob, i, GLP_IV);	//variables entières
	} 

	// coeficients de la fonction d'objectif
	//tout des cout reduit sont de 1
	for(i = 1;i <=  motifs.size();i++) glp_set_obj_coef(prob,i,1);  
	
	/* Définition des coefficients non-nuls dans la matrice des contraintes, autrement dit les coefficients de la matrice creuse */
	/* Les indices commencent également à 1 ! */
	
	ia = (int *) malloc ((1 + p.nbcreu) * sizeof(int)); //numero de la contrainte
	ja = (int *) malloc ((1 + p.nbcreu) * sizeof(int)); //numero de la variable
	ar = (double *) malloc ((1 + p.nbcreu) * sizeof(double)); //valeur du coef

	pos = 1;
	for(i = 0;i < p.nb ;++i) { //parcour des objets
		for(j = 0;j < motifs.size() ;++j) {
			if (motifs.at(j).at(i) != 0) { //si l'objet i est present dans le motif j
				ia[pos] = i + 1; //contrainte i => objet i
				ja[pos] = j + 1; //variable j => motif j
				ar[pos] = motifs.at(j).at(i); //quantite de l'objet i dans le motif j
				pos++;
			}
		}
	}
	
	/* chargement de la matrice dans le problème */
	
	glp_load_matrix(prob,p.nbcreu,ia,ja,ar); 
	
	/* Optionnel : écriture de la modélisation dans un fichier (utile pour debugger) */

	glp_write_lp(prob,NULL,"bin-packin.lp");

	/* Résolution, puis lecture des résultats */
	
	glp_simplex(prob,NULL);	glp_intopt(prob,NULL); /* Résolution */
	z = glp_mip_obj_val(prob); /* Récupération de la valeur optimale. Dans le cas d'un problème en variables continues, l'appel est différent : z = glp_get_obj_val(prob);*/
	x = (double *) malloc (p.nb * sizeof(double));
	for(i = 0;i < p.nb; i++) x[i] = glp_mip_col_val(prob,i+1); /* Récupération de la valeur des variables, Appel différent dans le cas d'un problème en variables continues : for(i = 0;i < p.nbvar;i++) x[i] = glp_get_col_prim(prob,i+1);	*/

	cout<<"il faut "<<z<<" bin"<<endl;
	for(i = 0; i< motifs.size(); ++i) {
		cout<<"motif"<<i<<" ( ";
		for(j = 0; j<p.nb; ++j) {
			cout<<motifs.at(i).at(j)<<" ";
		}
		cout<<") : "<<(int)(x[i] + 0.5)<<" ; ";
	} 
	puts("");


	/* Problème résolu, arrêt du chrono et affichage des résultats */

	double temps;
	crono_stop();
	temps = crono_ms()/1000,0;

	printf("Temps : %f\n",temps);	

		/* libération mémoire */
	glp_delete_prob(prob); 
	for(i = 0;i < p.nb;i++) free(p.tab[i]);
	free(ia);
	free(ja);
	free(ar);
	free(x);
											
	/* J'adore qu'un plan se déroule sans accroc! */
	return 0;
}
	
void trouve_motif ( vector <vector <int> > *motif, donnees *p) {
	//variable
	vector<int> *parc;
	int i,j;
	
	//triage des objet par taille decroissante
	sort(p->tab.begin(),p->tab.end(),compare);
	
	//recherche de l'ensemble des motifs possible
	//avoir au moin un elemnt est necessaire pour les initialisations de complete_motif
	for (i = 0; i <p->nb; ++i) {
		parc = new vector<int>();
		//on a pour base un motif par type d'objet
		//chaque base commence par un type d'objet différent
		//on suppose que tout les objets peuvent rentrer tout seul dans un bin
		for(j = 0; j<i; ++j) {parc->push_back(0);}
		parc->push_back(1);
		//remplissage a faon de toutes les maniere possible de cette base
		complete_motif(parc,p,motif);
		delete parc;
	}
}
void complete_motif ( vector <int> *base, donnees *p, vector <vector <int> > *motif) {
	//variable
	int fin = base->size()-1; //numero du dernier type d'objet ajouté, indice de la dernière case du vecteur *base, bien positif car la base à toujours au moins un element
	int taille = 0; //taille du motif de base
	vector <int> * nouv; //nouveau motif contruit a partir du motif de base
	int i,j;

	//debut

	//calcul de la taille du motif de base
	for (i = 0; i<=fin; ++i) {
		taille += (base->at(i) * p->tab.at(i)->t);
	}


	//le motif est fini si on ne peut rien ajouter
	if ( taille + (p->tab.at(p->nb-1)->t) > (p->T) ) {
		//on le remplit de 0
		for(i = fin+1; i<p->nb; ++i) {base->push_back(0);}
		//incrémentation du nombre de coef non nul de la matrice pleine
		for(i = 0; i<=fin; ++i) {p->nbcreu += (base->at(i) != 0);}
		//motif complet, il y a recopie donc pas supprimé lors du retour dans la fonction appelante
		motif->push_back(*base);
	} else {
		//essai d'ajout d'autre objet
		for (i = fin; i< p->nb; ++i) {
			nouv = new vector<int> (*base);
			//mettre des 0 jusqu'à l'objet ou l'on est rendu
			for (j = fin+1; j<i; ++j) {nouv->push_back(0);}
			
			if ( taille + p->tab.at(i)->t <= p->T) {
				//si on peut ajouter un objet de type i
				if (fin == i) {
					//si on travail sur un objet deja ajouté, i.e forcement le dernier
					nouv->back() = base->back() + 1;
				} else {
					//si on a pas deja ajouté d'objet de ce type
					nouv->push_back (1);
				}
				//si on a pas pus le mettre allors ce sera compté dans la completion du motif par des 0 au debut
				complete_motif(nouv,p,motif);
			}
			delete nouv;
		}
	}
	//fin
}