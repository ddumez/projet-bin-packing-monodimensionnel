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


int tours = 0;

typedef struct {
	int T; // Taille du bin 
	int nb; // Nombre de tailles différentes pour les objets
	vector <objets *> tab; // Tableau des objets (taille + nombre dans une même taille)
} donnees;
bool compare ( objets *a, objets *b) { return a->t > b->t;}
bool compare_int ( int a, int b) {return a > b;}
donnees * vers_p;
bool compare_motif( vector<int> a, vector<int> b) {
	int acc_a,acc_b;
	acc_a = 0; acc_b = 0;
	for(int i = 0; i<a.size(); ++i ) {
		acc_a += a.at(i) * vers_p->tab.at(i)->t;
		acc_b += b.at(i) * vers_p->tab.at(i)->t;
	}
	return acc_a > acc_b; //retourne si a est plus remplit que b
}

void lecture_data(char *file, donnees *p);
void trouve_motif ( vector <vector < int> > *motif, donnees *p);
void complete_motif ( vector < int> *base, donnees *p, vector <vector < int> > *motif);
int best_fit (donnees *p);
int * recherche (vector <vector < int> > *motifs, vector<objets> *reste,  int nbmax,  int base[],  int *nbtype, int deb, int tr, int nbb);
int max ( int a,  int b) {if (a<b) {return b;} else {return a;}}
int min ( int a,  int b) {if (b<a) {return b;} else {return a;}}



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


 
int main(int argc, char *argv[]) {
	/* Données du problème */
	donnees p; 
		
	// autres déclarations 
	vector < vector <int> > motifs; //vectur de l'ensembles des motifs : ligne numero du motif, colonne : nombre d'objet de ce type dans ce motif
	int nbmax; //nombre de bin max, i.e solution de best-fit
	vector<objets> reste;
	int i,j;
	/* Chargement des données à partir d'un fichier */

	lecture_data(argv[1],&p);
	reste.resize(p.nb);	//maintenant que l'on a le nombre d'objet
	/* Lancement du chronomètre! */
	crono_start();
	
	//trouver tout les bins possibles
	trouve_motif(&motifs, &p);
	int *soluce = new int[motifs.size()]; //maintenant que l'on a le nombre de motifs
	
	//recherche d'une borne supérieur pour le nombre de bin avec best-fit
	nbmax = best_fit(&p);
cout<<"best fit : "<<nbmax<<endl;
	//recherche de la solution optimale

	vers_p = &p;
	//trie des motifs pour commencer par tester le plus remplit, en general plus prometeur, et pour estimer rapidement et plus precisement le nombre de bin min restant a utiliser(dans recherche)
	sort(motifs.begin(),motifs.end(),compare_motif); //trie les motif par taille (somme des taille des objet dedant) par ordre decroissant, donc les plus remplit d'abort
	for(i = 0; i<motifs.size(); ++i) { soluce[i] = 0;} //aucun motif
	j = 0;
	for(i = 0; i<p.nb; ++i) { //parcour des objets
		reste.at(i).nb = p.tab.at(i)->nb; //aucun objet de mis => ils sont tous a mettre
		reste.at(i).t = p.tab.at(i)->t;
		j += p.tab.at(i)->nb * p.tab.at(i)->t; //on compte la taille totale
	}

	cout<<"pour ranger : ";
	for(i = 0; i<p.nb; ++i) {cout<<p.tab[i]->nb<<" ";}
	cout<<endl;

	//nbmax+1 pour qu'il cherche une solution en, au plus, nbmax
	//*motifs, *reste, nbmax, base, *nbtype, T, deb, tr, nbb);
	soluce = recherche (&motifs, &reste, nbmax+1, soluce, &(p.nb), 0, j, 0);

	//affichage des resultats
	if(0 == soluce) {
		cout<<"pas de solution"<<endl;
	} else {
		nbmax = 0;
		for(i = 0; i<motifs.size(); ++i) { nbmax += soluce[i];}
		cout<<"nombre de bin necessaires : "<<nbmax<<endl;
		for(i = 0; i<motifs.size(); ++i) {
			cout<<"motif"<<i<<"(";
			for(j = 0; j<p.nb; ++j) {cout<<motifs.at(i).at(j)<<" ";}
			cout<<") : "<<soluce[i]<<" ; ";
		}
		cout<<endl;
		delete [] soluce;
	}

	for(i = 0; i<p.nb; ++i) {delete p.tab.at(i);}
	crono_stop();
	cout<<"temps de calcul : "<< crono_ms()/1000 <<" secondes"<<endl;
	cout<<"nombre de noeud parcourus : "<<tours<<endl;
	/* J'adore qu'un plan se déroule sans accroc! */
	return 0;
}

void lecture_data(char *file, donnees *p) {
	
	FILE *fin; // Pointeur sur un fichier
	int i;	
	int val;
	objets *tmp;
	
	fin = fopen(file,"r"); // Ouverture du fichier en lecture
	
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
					nouv->pop_back();
					nouv->push_back(base->back() + 1);
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


int best_fit (donnees *p) {
	//variables
		vector <int> soluce; //solution que l'on vas construire, contien la place restante
		int i,j,parc;
	//debut
	//le tableau des objet, dans p, a deja été trié dans la recherche de motif
	
	for(i = 0; i<p->nb; ++i) { //parcour des objets
		for(j = 0; j<p->tab.at(i)->nb ; ++j) { //autant de fois que ce type d'objet est present
			sort(soluce.begin(), soluce.end(), compare_int ); //on trie les bin pour ajouter sur les plus remplit
			parc = 0;
			while ( (parc < soluce.size()) && (soluce.at(parc) + p->tab.at(i)->t > p->T)) {++parc;} //recherche du premier bin ou l'on peut mettre l'objet, condition ne plante pas grace à l'évaluation flemarde
			if (soluce.size() == parc ) {
				//aucun bin actuel ne peut le contenier donc on en cree un nouveau
				soluce.push_back(p->tab.at(i)->t);
			} else {
				//ajout dans le premier bin qui peut le contenir
				//mise a jour de la valeur de cette case
				soluce.at(parc) += p->tab.at(i)->t;
			}
		}
	}
	
	//fin
	return soluce.size(); //on retourne le nombre de bin utilisé pour ranger tous les objets
}


/*
motifs : vecteur de l'ensemble des motifs
reste : tableau des objet qu'il est a placer
nbmax : nombre maximum de bin possible
base : solution en cour de traitement
nbtype : nombre de type d'objet
deb : premier motif à considerer, les precedent on deja été utilisé
tr : taille des objet restant, i.e somme des tailles des objet non deja dans des bins
nbb : nombre de bin utilisé dans la base
*/
int * recherche (vector <vector < int> > *motifs, vector<objets> *reste,  int nbmax,  int base[],  int *nbtype, int deb, int tr, int nbb) {
 	//variables
	int i;
++tours;

	//test si on a fini

	if (0 == tr) {
		//il ne reste rien donc on a fini
		return base;
	} else { //on continue
		//variables
		int *soluce;
		int *soluce_m = 0; //nouvelle solutions que l'on vas construire en completant la base
		vector<objets> reste_c (*nbtype); //pour la recopie du reste pour les appels recursifs
		int tm = 2147483646; //MAXINT
		bool est_utile, parc_utile;
		int parc, parc2;
		int tmp; // variable de calcul

		//début

		++nbb;
		//on parcour tous les motifs à partir du dernier ajouté
		for(i = deb; i<motifs->size(); ++i) {
			//on est dans le motif i
			est_utile = false; parc = 0;
			while( ( !est_utile ) && (parc <*nbtype) ) {
				est_utile = (reste->at(parc).nb > 0) && (motifs->at(i).at(parc) > 0); //il y a des objets restant present dans le motif
				++parc;
			}

			if (est_utile) {

				//mise à jour du nombre d'objet restant dans une copie
				tmp = 0; parc2 = 0;
				for(parc = 0; parc<*nbtype; ++parc) {
					reste_c.at(parc).nb = max(0, reste->at(parc).nb - motifs->at(i).at(parc));
					reste_c.at(parc).t = reste->at(parc).t;
					tmp += reste_c.at(parc).nb * reste_c.at(parc).t; //en meme temps on calcule la taille de ce qui reste
					parc2 += motifs->at(i).at(parc) * reste_c.at(parc).t; //on en profite pour calculer la taille du bin restant le plus remplit
				}

				//il est utile d'utiliser ce motif donc on parcour cette branche
				
				//de temps en temps, a environ 1/4, 1/2 et 3/4 de la profondeur estimé de l'arbre, on re-affine notre estimation de majoration sur ce qui reste avec bes-fit
				if (( (int)(nbmax/4) == nbb ) || ( (int)(nbmax/2) == nbb ) || ( (int)(3*nbmax/4) == nbb ) ) { 
					for(parc = 0; parc<*nbtype; ++parc) {vers_p->tab.at(parc)->nb = reste_c.at(parc).nb;}
					nbmax = min( nbmax,best_fit(vers_p)+nbb);
				}

				//on verifie rapidement si cette branche est une impasse
				//estimation affiné : on ne prend plus p.T qui est la taille max mais la taille du bin restant le plus remplit
				est_utile = !(( nbb + (tmp / parc2)) > min(nbmax,tm-1)); //si on a au mieux une solution avec 1 bin de moins que celle que l'on à deja

				//si il reste des objets qui ne sont present dans aucun bin restant
				parc = 0;
				while ( est_utile && (parc < *nbtype)) {
					if (reste_c.at(parc).nb > 0) { //si il reste de cet objet
						parc2 = i; parc_utile = false;
						//on regarde si un des motif restant contien cet objet
						while( ( !parc_utile ) && (parc2 < motifs->size()) ) { parc_utile = (motifs->at(parc2).at(parc) > 0); ++parc2;}
						est_utile = est_utile && parc_utile;
					}
					++parc;
				}
				if(est_utile) {
					//recopie des donne a modifié pour l'appel recursif
					soluce = new int[motifs->size()]; //la case i corespond au nombre de motif i utilisé
					for(parc = 0; parc<motifs->size(); ++parc) { soluce[parc] = base[parc];} //recopie de la base

					++ soluce[i]; //on compte le bin que l'on vient de rajouter, motif de type i

					//appel recursif
					//soluce est desaloué dans l'appel recursif si besoin

					soluce = recherche (motifs, &reste_c, min(tm-1,nbmax), soluce, nbtype, i, tmp, nbb);

					//mise à jour du resultat
					if (soluce != 0) {
						//si cette branche a été parcourue
						tmp = 0;
						//on compte le nombre de bin utilisé
						for(parc = 0; parc<motifs->size(); ++parc) { tmp += soluce[parc];}
						if ( tmp < tm ) {
							//si cette solution est meilleure que notre precedente meilleure
							tm = tmp;
							//on supprime la precedent meilleure solution car elle devient inutile
							if(0 != soluce_m)  {delete [] soluce_m ;} //pour eviter un seg-fault si c'est la première solution
							soluce_m = soluce; //on met a jour la meilleure solution
						} else {
							delete [] soluce;//sinon on la suprime car elle est inutile, seule la meilleure est conservé
						}
					}
				}
			}
		}
		//fin
		delete [] base;
		return soluce_m;
	}
}
