/**** Includes ****/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>




/**** Defines ****/

#define DECKSIZE 52					// the number of cards in a deck
#define HANDSIZE 5					// the number of cards in a hand
#define SUITS 4						// number of suits in the deck
#define RANKS 13					// number of ranks in the deck

#define DISPLAY_REALTIME 1				// set to 1 to display odds after each iteration; 0 to display once at end. disabling improves speed by roughly a factor of forty.
#define DISPLAY_REALTIME_FREQ 1000000	// frequency of realtime display updates in hands.




/**** Typedefs ****/

typedef struct {
	int rank; 
	int suit; 
} card;




/**** Globals ****/

/* Cards */
card deck[DECKSIZE] = {0,0};	// the deck
card* hand[HANDSIZE] = {NULL};	// the hand; points to cards in the deck
unsigned drawnTable[DECKSIZE] = {0};	// hash table for whether the corresponding card has been drawn from the deck; hashed by (rank*4)+suit

/* Suits/ranks */
char suits[] = {'C', 'H', 'S', 'D'}; // array of all card suits
char* rankNames[] = {"ace", "deuce", "three", "four", "five", "six", 	// array of all card ranks
	"seven", "eight", "nine", "ten", "jack", "queen", "king"}; 

/* Lowest/highest rank */
int lowestRankInHand = RANKS;	// the lowest rank in the hand
int highestRankInHand = 0;		// the highest rank in the hand

/* Poker hand occurrances */ // global to avoid passing all of them to a display function
unsigned onePair = 0, twoPair = 0, 	// number of ocurrances of this hand
	threeOfAKind = 0, fourOfAKind = 0,
	straight = 0, flush = 0, fullHouse = 0;

/* Count tables */
unsigned countTable_suit[SUITS] = {0}; 		// The count of each suit in the hand; hashed by suit (asc)
unsigned countTable_rank[RANKS] = {0}; 		// The count of each rank in the hand; hashed by rank (asc)

/* X-of-a-kind tables */
unsigned xOfAKindTable_suit[DECKSIZE] = {0};	// The count of x-of-a-kind occurances of suits in the hand; hashed by x (as in x-of-a-kind) (asc)
unsigned xOfAKindTable_rank[DECKSIZE] = {0};	// The count of x-of-a-kind occurances of rank in the hand; hashed by x (as in x-of-a-kind) (asc)




/**** Function Prototypes ****/

void simulatePokerOdds(unsigned, int);

void dealHand(card*, card**);
void countHand(card**);
void returnHand(card**, card*);
unsigned isXofAKind_rank(int);
unsigned isXofAKind_suit(int);

int isStraight();
int isFlush();
int isFullHouse();
int isOnePair();
int isTwoPair();
int isThreeOfAKind();
int isFourOfAKind();

void displayOdds(unsigned);
void displayStatus(time_t, time_t, unsigned, unsigned);
void arrangeHand(card**);
void resetCounts();
void initialize(card*);
void shuffle(card*);
void swapDeck(card*, card*);
void swapHand(card**, card**);
void inputNumHands(unsigned*);
int isDrawn(card*);
void setDrawn(card*, int);



/**** Main ****/

int main(void)
{
	int draws;	// number of hands to draw. initialized with user input

	// input number hands to deal
	inputNumHands(&draws);

	// simulate number of hands
	simulatePokerOdds( draws, DISPLAY_REALTIME );

	// exit gracefully
	puts("\n");
	return EXIT_SUCCESS;
}


/**** Function Definitions ****/

/**
 * @brief simulate drawing a number of hands and report derived poker hand odds. 
 * @param draws - the number of draws to simulate.
 * @param realtimeDisplay - whether to enable realtime display mode
 */
void simulatePokerOdds(unsigned draws, int realtimeDisplay)
{
	unsigned totalHandsDealt = 0;	// number of hands dealt

	// seed random number stream
	srand((unsigned) time(NULL));

	// fill deck
	initialize(deck);

	// start timers
	time_t startTime = time(NULL);					// the time at the start of the simulation
	time_t realtimeDisplayTimer = startTime;// - 100;	// display cooldown - subtracting triggers immediate display

	// print header row
	printf("\n%-8s  %-8s  %-8s  %-8s  %-8s  %-8s  %-8s \t%-8s  %-8s \n\n"
		, "----1P----" , "----2P----" , "----3K----"
		, "----4K----" , "-Straight-" , "--Flush---"
		, "-F.-House-" , "  % Dealt " , "Time elapsed");

	// deal hands
	time_t currentTime;		// cache start time time
	while (++totalHandsDealt < draws+1)
	{
		/*--------- calculate odds ---------*/

		// deal a new hand
		// shuffle(deck);		// shuffle the deck and hand simultaneously 	// not needed since the hand is drawn randomly instead
		dealHand(deck, hand);	// deal a hand from the deck
		// arrangeHand(hand); 	// sort the hand in order of rank	// not necessary since count tables are used 
		countHand(hand);		// count occurrances of each rank and suit as well as x-of-a-kind occurrances

		// increment hand ocurrances
		onePair += isOnePair();
		twoPair += isTwoPair();
		threeOfAKind += isThreeOfAKind();
		fourOfAKind += isFourOfAKind();
		straight += isStraight();
		flush += isFlush();
		fullHouse += isFullHouse();

		// return hand to deck
		returnHand(hand, deck);

		/*--------- display odds ---------*/
		#if (DISPLAY_REALTIME)
		// only update odds display every DISPLAY_REALTIME_FREQ iteration; skip display if cooldown period has not elapsed
		// delay used to speed up output for large tests (displaying each iteration reduces iteration speed by about a factor of 40)
		if ( !(totalHandsDealt % DISPLAY_REALTIME_FREQ) )
		{
			// update odds display with time elapsed
			displayOdds(totalHandsDealt);
			displayStatus( time(&currentTime), startTime, totalHandsDealt, draws);
			fflush(stdout);	// fflush to force immediate output. omitting causes irregular display updates
		}
		#endif
	}

	// display final odds
	displayOdds(totalHandsDealt);
	displayStatus(time(NULL), startTime, totalHandsDealt, draws);
}

/**
 * @brief draws random cards from the deck into the hand.
 * @param deck - the source deck
 * @param hand - the hand to fill
 */
void dealHand(card* deck, card** hand)
{
	for (card **i = hand; i < hand+HANDSIZE; ++i)
	{
		// draw a random card, with loop to prevent drawing the same card twice
		do
		{
			// pick a random card from the (unsorted) deck
			*i = deck + ( rand() % DECKSIZE );

		// until an undrawn card is found 
		} while ( isDrawn(*i) );

		// set card to drawn and add to hand
		setDrawn(*i, 1);
	}
}

/**
 * @brief returns a hand to the deck.
 * @param hand - the hand to return
 * @param deck - the deck receiving the hand
 */
void returnHand(card** hand, card* deck)
{
	unsigned* isDrawn;
	// return each card in hand to deck
	for (card **i = hand; i < hand+HANDSIZE; ++i)
	{
		// reset isDrawn value in hash table to zero 
		setDrawn(*i, 0);
	}
}

/**
 * @brief count the cards in a hand and caches statistics including:
 * -count of each rank/suit
 * -count of xOfAKinds of rank/suit
 * -highest/lowest ranking card in hand
 * @param hand - the hand to count.
 */
void countHand(card** hand)
{
	// reset counts 
	resetCounts();

	// count cards in hand
	int lowestRank = RANKS;
	int highestRank = 0;
	for (card **i = hand; i < hand+HANDSIZE; ++i)
	{
		// add card to count 
		++( *(countTable_suit + ((*i)->suit - 1) ) );
		++( *(countTable_rank + ((*i)->rank - 1) ) );

		// find lowest/highest rank
		if ((*i)->rank < lowestRank) lowestRank = (*i)->rank;
		if ((*i)->rank > highestRank) highestRank = (*i)->rank;
	}

	// use count table to count x-of-a-kind of all ranks
	for (unsigned *i = countTable_rank, *j = xOfAKindTable_rank; i < countTable_rank+RANKS; ++i)
		( *( j + *i) )++;

	// use count table to count x-of-a-kind of all suits
	for (unsigned *i = countTable_suit, *j = xOfAKindTable_suit; i < countTable_suit+SUITS; ++i)
		( *( j + *i) )++;

	// commit lowest/highest rank to global
	lowestRankInHand = lowestRank;
	highestRankInHand = highestRank;
}

/**
 * @brief checks a hand for x-of-a-kind by rank.
 * @param x - the number of matching cards required to return true (x-of-a-kind)
 * @return the number of x-of-a-kind sets found,
 *  eg. [1,2,3,4], x = 2 : 0
 *  eg. [1,1,3,4], x = 2 : 1
 *  eg. [1,1,2,2], x = 2 : 2
 */
unsigned isXofAKind_rank(int x)
{
	return ( *( xOfAKindTable_rank + x ) );
}

/**
 * @brief checks a hand for x-of-a-kind by suit.
 * @param x - the number of matching cards required to return true (x-of-a-kind)
 * @return the number of x-of-a-kind sets found,
 *  eg. [1,2,3,4], x = 2 : 0
 *  eg. [1,1,3,4], x = 2 : 1
 *  eg. [1,1,2,2], x = 2 : 2
 */
unsigned isXofAKind_suit(int x)
{
	return ( *( xOfAKindTable_suit + x ) );
}



/**
 * @brief checks for One Pairs in the count table.
 * @return 1 if a One Pair is found, 0 if none are found
 */
int isOnePair()
{
	return isXofAKind_rank(2) == 1;
}

/**
 * @brief checks for Two Pairs in the count table.
 * @return 1 if a Two Pair is found, 0 if none are found
 */
int isTwoPair()
{
	return isXofAKind_rank(2) == 2;
}

/**
 * @brief checks for Three of a Kind in the count table.
 * @return 1 if Three of a Kind are found, 0 if none are found
 */
int isThreeOfAKind()
{
	return isXofAKind_rank(3) == 1;
}

/**
 * @brief checks for Four of a Kind in the count table.
 * @return 1 if Four of a Kind are found, 0 if none are found
 */
int isFourOfAKind()
{
	return isXofAKind_rank(4) == 1;
}

/**
 * @brief checks for a straight in the count table.
 * @return 1 if a straight is found, 0 if not
 */
int isStraight()
{
	// early out if there are any non-unique ranks
	if (isXofAKind_rank(1) != HANDSIZE) return 0;

	// since all cards in hand must have unique ranks at this point, the hand must be a straight if the lowest rank is exactly HANDSIZE apart from the highest rank
	return ( (highestRankInHand - lowestRankInHand + 1) == HANDSIZE );
}

/**
 * @brief checks for a flush in the count table.
 * @return 1 if a flush is found, 0 if not
 */
int isFlush()
{
	// if there are HANDSIZE-of-a-kind of a suit, the hand is a flush
	return isXofAKind_suit(HANDSIZE) > 0;
}

/**
 * @brief checks for a Full House in the count table.
 * @return 1 if a Full House is found, 0 if not
 */
int isFullHouse()
{

	return isXofAKind_rank(3) && isXofAKind_rank(2);
}




/**
 * @brief displays the cumulative odds against each poker hand based on the cards drawn.
 * @param handsDealt - the number of hands dealt
 */
void displayOdds(unsigned handsDealt)
{
	printf("\r%5u:1     %5u:1     %5u:1      %5u:1     %5u:1     %5u:1     %5u:1 \t"
		, onePair ? (handsDealt / onePair) : 0
		, twoPair ? (handsDealt / twoPair) : 0
		, threeOfAKind ? (handsDealt / threeOfAKind) : 0
		, fourOfAKind ? (handsDealt / fourOfAKind) : 0
		, straight ? (handsDealt / straight) : 0
		, flush ? (handsDealt / flush) : 0
		, fullHouse ? (handsDealt / fullHouse) : 0);
}

/**
 * @brief displays the progress and time elapsed of a simulation.
 * @param currentTime - the current time
 * @param startTime - the start time of the simulation
 * @param handsDealt - number of hands dealt so far
 * @param numHands - total number of hands being dealt
 */
void displayStatus(time_t currentTime, time_t startTime, unsigned handsDealt, unsigned numHands)
{
	printf("  %4u%%     [ %u seconds ]"
		, (unsigned) ((float) handsDealt / (float) numHands * 100.f)
		, (unsigned) (currentTime-startTime) );
}

/** 
 * @brief Sorts the hand array.
 * @param hand - the hand to sort
 */
void arrangeHand(card** hand)
{
	card** pMax;
	for (card **i = hand; i < hand+HANDSIZE; ++i)
	{
		pMax = i;
		for (card **j = i; j < hand+HANDSIZE; ++j)
			if ((*j)->rank > (*pMax)->rank) pMax = j;

		swapHand(i, pMax);
	}
}

/**
 * @brief Sets all counts to zero.
 */
void resetCounts()
{
	// ranks
	for (unsigned *i = countTable_rank; i < countTable_rank+RANKS; ++i)
	{
		// reset x-of-a-kind
		( *(xOfAKindTable_rank + *i) ) = 0;

		// reset count
		*i = 0;
	}

	// suits
	for (unsigned *i = countTable_suit; i < countTable_suit+SUITS; ++i)
	{
		// reset x-of-a-kind
		( *(xOfAKindTable_suit + *i) ) = 0;

		// reset count
		*i = 0;
	}

	// reset lowest/highest rank to global
	lowestRankInHand = 0;
	highestRankInHand = 0;
}

/**
 * @brief fills a deck of cards.
 * @param deck - the deck to fill
 */
void initialize(card* deck){
   for (int crdctr = 0; crdctr < DECKSIZE; deck++, crdctr++){
	  deck->rank = crdctr%13 + 1;
	  deck->suit = crdctr%4 + 1;
   }
}

/**
 * @brief shuffles a deck of cards.
 * @param deck - the deck to shuffle
 */
void shuffle(card* deck){
   int swapCard;
   for (int crdctr = DECKSIZE-1; crdctr > 0; --crdctr){
	  swapCard = rand() % (crdctr+1);
	  swapDeck(deck+crdctr, deck+swapCard);
   }
}

/**
 * @brief swaps two cards in a deck.
 * @param p - the first card to swap
 * @param q - the second card to swap
 */
void swapDeck(card* p, card* q){
   card tmp;
   tmp = *p;
   *p = *q;
   *q = tmp;
}

/**
 * @brief swaps two cards in a hand.
 * @param p - the first card to swap
 * @param q - the second card to swap
 */
void swapHand(card** p, card** q){
	card* tmp;
	tmp = *p;
	*p = *q;
	*q = tmp;
}

/**
 * @brief helper function to allow the user to input the number of hands to draw
 * @param pDraws - ptr to the variable to fill with user input
 */
void inputNumHands(unsigned* pDraws)
{
	// input num hands to deal
	printf("How many hands would you like to deal? ");
	scanf("%u", pDraws);
}

/**
 * @brief find whether a particular card has been drawn from the deck.
 * @param c - the card to check for
 * @return whether or not the card has been drawn
 */
int isDrawn(card* c)
{
	int result = *( drawnTable + ( (c->rank-1) * 4 ) + ( c->suit-1 ) );
	return *( drawnTable + ( (c->rank-1) * 4 ) + ( c->suit-1 ) );
}

/**
 * @brief set whether a particular card has been drawn from the deck.
 * @param c - the card to affect
 * @param drawn - whether or not the card has been drawn (true or false)
 * @return 
 */
void setDrawn(card* c, int drawn)
{
	*( drawnTable + ( (c->rank-1) * 4 ) + ( c->suit-1 ) ) = drawn;
}