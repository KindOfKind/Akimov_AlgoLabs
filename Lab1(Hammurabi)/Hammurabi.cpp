#define _CRT_SECURE_NO_WARNINGS 1 

#include <iostream>
#include <cmath>
#include <fstream>

#define DEFAULT_ROUNDS_NUM 10;

#define DEFAULT_INITIAL_ROUND 1
#define DEFAULT_POPULATION 100
#define DEFAULT_FOOD 2800
#define DEFAULT_LAND 1000

#define DEFAULT_SAVE_FILE_PATH "./SaveFile.txt"

enum class ERoundOutcomes {continuation, victory, starvationDefeat, rulingDefeat};
enum class ERulingQuality {awful, intermediate, good, great};

struct GameRules
{
	int roundsNum = DEFAULT_ROUNDS_NUM;
	int foodToFeedOnePerson = 20;
	int maxLandPerPerson = 10;
	float foodToSowLandUnit = 0.5f;
	float maxFoodPartSpoiledByRats = 0.07f;
	float plagueChance = 0.15f;

	int minLandPrice = 17;
	int maxLandPrice = 26;

	int minHarvestedFood = 1;
	int maxHarvestedFood = 6;

	int minNewPeopleNum = 0;
	int maxNewPeopleNum = 50;
};


class GameState
{
	int round = 0;
	int food = 0;
	int population = 0;
	int land = 0;

	float averageDeathsPerRound = 0;
	float deathsPartsSum = 0;			// Требуется для вычисления среднегодового процента умерших от голода
	float landPerPerson = 0;

	class RoundState
	{
	public:

		int landUnitPrice = 10;
		int deathsFromStarvation = 0;
		int deathsFromPlague = 0;
		int newPeopleNum = 0;
		int foodHarvested = 0;
		int foodFromLandUnit = 0;
		int foodSpoiledByRats = 0;

		RoundState() {};

		/*int GetLandUnitPrice() { return landUnitPrice; };
		int GetDeadPeopleNum() { return deadPeopleNum; };
		int GetNewPeopleNum() { return newPeopleNum; };
		int GetHarvestNum() { return harvestNum; };
		int GetFoodFromLandUnit() { return foodFromLandUnit; };
		int GetFoodSpoiledByRats() { return foodSpoiledByRats; };
		bool CheckIfPlagueOccured() { return bPlagueOccured; };*/
	};

public:
	
	GameState() {};

	GameState(int arg_round, int arg_food, int arg_population, int arg_land) :
	round(arg_round), food(arg_food), population(arg_population), land(arg_land) {};

	RoundState rS;

	int GetRound() { return round; };
	int GetFood() { return food; };
	int GetPopulation() { return population; };
	int GetLand() { return land; };

	float GetAverageDeathsPerRound() { return averageDeathsPerRound; };
	float GetDeathsPartsSum() { return deathsPartsSum; }
	float GetLandPerPerson() { return landPerPerson; };
	
	void IncrementRound() { round++; };

	void SetRound(int arg_round) { round = arg_round; };
	void SetFood(int arg_food) { food = arg_food; };
	void SetPopulation(int arg_population) { population = arg_population; };
	void SetLand(int arg_land) { land = arg_land; };
	
	void SetAverageDeathsPerRound(float arg_avDeaths) { averageDeathsPerRound = arg_avDeaths; };
	void SetDeathsPartsSum(float arg_deathsPartsSum) { deathsPartsSum = arg_deathsPartsSum; };
	void SetLandPerPerson(float arg_landPerPreson) { landPerPerson = arg_landPerPreson; };
};


int CinUnsignedIntWithValidation()
{
	int input = 0;
	while (true)
	{
		std::cin >> input;

		if (!std::cin.fail())
		{
			if (input >= 0)
				return input;
			else
				std::cout << "Возможен ввод только положительных чисел." << std::endl;
		}
		else
			std::cout << "Возможен ввод только целых чисел." << std::endl;
		std::cin.clear();
	}
}


class GameMode
{
	int peopleFed = 0;
	int landSowed = 0;
	int populationAtRoundStart = 0;

	ERoundOutcomes roundOutcome = ERoundOutcomes::continuation;
	ERulingQuality rulingQuality = ERulingQuality::good;

	GameRules* gR;
	GameState* gS;

public:

	GameMode(GameRules* arg_gR, GameState* arg_gS) : gR(arg_gR), gS(arg_gS) {};

	void MakeRequests();

	ERoundOutcomes ProcessRound();
	void ShowInitialMessage();
	void ShowRoundStatistics();

	void CountDeadOfStarvation();
	void IncreaseCityPopulation();
	void CountDeadFromPlague();
	void CountHarvestedFood();
	void CountFoodSpoiledByRats();
	void CalculateNewLandPrice();

	void CalculateRulingQuality();

	bool SaveGame(const std::string&);
	bool LoadGame(const std::string&);
};

void GameMode::MakeRequests()
{
	int landToBuy = 0;
	int foodToEat = 0;
	int landToSow = 0;
	int foodAfterRequest;

	// Покупаем землю
	do {
		std::cout << "\n\nСколько акров вы хотели бы купить?" << std::endl;
		landToBuy = CinUnsignedIntWithValidation();

		foodAfterRequest = gS->GetFood() - (landToBuy * gS->rS.landUnitPrice);

		if (foodAfterRequest < 0)
			std::cout << "Пытаетесь купить " << landToBuy << " акров земли, хотя у нас есть только " << gS->GetFood()
																	<< " бушелей пшена? Надеюсь, вы так шутите, мой господин..." << std::endl;
	}
	while (foodAfterRequest < 0);

	gS->SetLand(gS->GetLand() + landToBuy);
	gS->SetFood(foodAfterRequest);


	// Кормим людей
	do {
		std::cout << "Сколько бушелей пшена использовать для обеспечения продовольствия?" << std::endl;
		foodToEat = CinUnsignedIntWithValidation();

		foodAfterRequest = gS->GetFood() - foodToEat;

		if (foodAfterRequest < 0)
			std::cout << "Хотите использовать " << foodToEat << " бушелей пшена, хотя у нас есть только " << gS->GetFood()
			<< "? Не уверен, что понимаю вашего юмора, милорд..." << std::endl;
	} while (foodAfterRequest < 0);

	peopleFed = foodToEat / gR->foodToFeedOnePerson;
	gS->SetFood(foodAfterRequest);
	

	// Засеиваем землю
	do {
		std::cout << "Сколько акров будем засеивать?" << std::endl;
		landToSow = CinUnsignedIntWithValidation();

		foodAfterRequest = gS->GetFood() - (landToSow * gR->foodToSowLandUnit);

		if (foodAfterRequest < 0)
			std::cout << "Хотите засеять " << landToSow << " акров земли, хотя у нас есть только " << gS->GetFood()
			<< " бушелей пшена? Боюсь, план слишком амбициозный. Почему бы нам не оставить его до следующего года?\n\n" << std::endl;
	} while (foodAfterRequest < 0);

	landSowed = landToSow / gR->foodToSowLandUnit;
	gS->SetFood(foodAfterRequest);

}

void GameMode::CountDeadOfStarvation()
{
	int deadPeopleNum = std::max(0, gS->GetPopulation() - peopleFed);
	int peopleAlive = gS->GetPopulation() - deadPeopleNum;
	
	if (gS->GetPopulation() * 0.45 < deadPeopleNum) roundOutcome = ERoundOutcomes::starvationDefeat;

	gS->rS.deathsFromStarvation = deadPeopleNum;
	gS->SetPopulation(peopleAlive);
}

void GameMode::CountFoodSpoiledByRats()
{
	int chance = float(rand()) / float((RAND_MAX)) * gR->maxFoodPartSpoiledByRats;
	int spoiledFood = chance * gS->GetFood();

	gS->rS.foodSpoiledByRats = spoiledFood;
	gS->SetFood(gS->GetFood() - spoiledFood);
}

void GameMode::CalculateNewLandPrice()
{
	int minMaxDif = gR->maxLandPrice - gR->minLandPrice;
	int newLandPrice = (int)(rand() % (minMaxDif + 1)) + gR->minLandPrice;

	gS->rS.landUnitPrice = newLandPrice;
}

void GameMode::CountHarvestedFood()
{
	int minMaxDif = gR->maxHarvestedFood - gR->minHarvestedFood;
	int foodFromLandUnit = (int)(rand() % (minMaxDif + 1)) + gR->minHarvestedFood;
	int foodHarvested = foodFromLandUnit * landSowed;

	gS->rS.foodFromLandUnit = foodFromLandUnit;
	gS->rS.foodHarvested = foodHarvested;

	gS->SetFood(gS->GetFood() + foodHarvested);
}

void GameMode::IncreaseCityPopulation()
{
	int newPeopleNum = gS->rS.deathsFromStarvation / 2 + (5 - gS->rS.foodFromLandUnit) * gS->GetFood() / 600 + 1;
	newPeopleNum = std::max(gR->minNewPeopleNum, std::min(newPeopleNum, gR->maxNewPeopleNum));

	gS->SetPopulation(gS->GetPopulation() + newPeopleNum);
	gS->rS.newPeopleNum = newPeopleNum;
}

void GameMode::CountDeadFromPlague()
{
	int deathsFromPlague = 0;

	bool plagueOccured = rand() / (RAND_MAX + 1.0f) < gR->plagueChance;
	if (plagueOccured)
	{
		deathsFromPlague = std::floor(gS->GetPopulation() / 2);
	}

	gS->SetPopulation(gS->GetPopulation() - deathsFromPlague);
	gS->rS.deathsFromPlague = deathsFromPlague;
}

ERoundOutcomes GameMode::ProcessRound()
{
	gS->IncrementRound();
	if (gS->GetRound() == gR->roundsNum)
	{
		roundOutcome = ERoundOutcomes::victory;
		return roundOutcome;
	}

	populationAtRoundStart = gS->GetPopulation();

	CalculateNewLandPrice();
	CountHarvestedFood();
	CountFoodSpoiledByRats();
	CountDeadOfStarvation();
	IncreaseCityPopulation();
	CountDeadFromPlague();
	CalculateRulingQuality();

	if (roundOutcome == ERoundOutcomes::continuation) SaveGame(DEFAULT_SAVE_FILE_PATH);

	return roundOutcome;
}

void GameMode::ShowInitialMessage()
{
	std::cout << "Добро пожаловать на трон, мой господин!\n";
	std::cout << "В нашем городе людей ровно " << gS->GetPopulation() << ".\nВ амбарах знаете, сколько бушелей? Аж " << gS->GetFood() << "!\n";
	std::cout << "Акров в нашем городе " << gS->GetLand() << ", а стоимость 1-го акра в бушелях " << gS->rS.landUnitPrice << ".\nНадеюсь, что с вами город расцветёт пуще прежнего!";
}

void GameMode::ShowRoundStatistics()
{

	switch (roundOutcome)
	{
	case (ERoundOutcomes::continuation):

		std::cout << "Год правления: " << gS->GetRound() << "\nУ нас " << gS->rS.deathsFromStarvation << " умерших с голоду, " << gS->rS.newPeopleNum << " новеньких.\n";
		if (gS->rS.deathsFromPlague > 0) std::cout << gS->rS.deathsFromPlague << " погибших от чумы\n";
		std::cout << "В городе людей ровно " << gS->GetPopulation() << std::endl << "Бушелей пшеницы прибавилось ровно " << gS->rS.foodHarvested << ", с акра по " << gS->rS.foodFromLandUnit << std::endl <<
			"Бушелей, истреблённых крысами, у нас " << gS->rS.foodSpoiledByRats << ", потому в амбарах остаётся " << gS->GetFood() << std::endl;
		std::cout << "Акров в нашем городе теперь " << gS->GetLand() << ", а стоимость 1-го акра в бушелях " << gS->rS.landUnitPrice << std::endl;
		break;

	case (ERoundOutcomes::victory):

		switch (rulingQuality)
		{
		case (ERulingQuality::intermediate):
			std::cout << "Надеюсь, такие, как вы, больше не будут править нашим городом.\n";
			break;
		case (ERulingQuality::good):
			std::cout << "Очень неплохо. Полагаю, можно с уверенностью сказать, что нам повезло с правителем.\n";
			break;
		case (ERulingQuality::great):
			std::cout << "Лучшего правителя нам было бы не найти! О вашем имени обязательно станет известно всему миру.\n";
			break;
		}
		break;

	case (ERoundOutcomes::starvationDefeat):

		std::cout << "Слишком много людей умерло от голода. Теперь самому бы как-нибудь выжить, куда уж нам думать о других...\n";
		break;

	case (ERoundOutcomes::rulingDefeat):

		std::cout << "Боже, за что же ты послал нам этого правителя? Что есть причина гневу твоему?..\n";
		break;
	}

	
}

void GameMode::CalculateRulingQuality()
{
	float deadPartInThisRound = (float)gS->rS.deathsFromStarvation / populationAtRoundStart;
	float deathsPartsSum = gS->GetDeathsPartsSum() + deadPartInThisRound;

	float avDeaths = deathsPartsSum / (gS->GetRound() - 1);
	gS->SetDeathsPartsSum(deathsPartsSum);
	gS->SetAverageDeathsPerRound(avDeaths);

	float landPerPerson = (float)gS->GetLand() / gS->GetPopulation();
	gS->SetLandPerPerson(landPerPerson);

	if (avDeaths > .33 || landPerPerson < 7)
	{
		roundOutcome = ERoundOutcomes::rulingDefeat;
		rulingQuality = ERulingQuality::awful;
	}
	else if (avDeaths > .1 || landPerPerson < 9)
		rulingQuality = ERulingQuality::intermediate;
	else if (avDeaths > .03 || landPerPerson < 10)
		rulingQuality = ERulingQuality::good;
	else
		rulingQuality = ERulingQuality::great;

	std::cout << "\n\n" << "Среднегодовой процент умерших: " << avDeaths << "\nКоличество акров земли на одного жителя: " << landPerPerson << "\n\n";
}


bool GameMode::SaveGame(const std::string& saveFilePath)
{
	std::fstream fs(saveFilePath, std::fstream::out);

	if (!fs) return false;

	fs << gS->GetRound() << '\n' << gS->GetFood() << '\n' << gS->GetPopulation() << '\n' << gS->GetLand() << '\n' <<
		gS->GetAverageDeathsPerRound() << '\n' << gS->GetDeathsPartsSum() << '\n' << gS->GetLandPerPerson() << '\n' <<
		gS->rS.landUnitPrice << '\n' << gS->rS.deathsFromStarvation << '\n' << gS->rS.deathsFromPlague << '\n' << gS->rS.newPeopleNum << '\n' <<
		gS->rS.foodHarvested << '\n' << gS->rS.foodFromLandUnit << '\n' << gS->rS.foodSpoiledByRats << '\n'
		<< peopleFed << '\n' << landSowed << '\n' << populationAtRoundStart;
}

bool GameMode::LoadGame(const std::string& saveFilePath)
{
	std::fstream fs(saveFilePath, std::fstream::in);

	if (!fs) {
		return false;
	}

	int temp;
	float tempF;

	fs >> temp; gS->SetRound(temp);					fs >> temp; gS->SetFood(temp);				fs >> temp; gS->SetPopulation(temp);	fs >> temp; gS->SetLand(temp);
	fs >> tempF; gS->SetAverageDeathsPerRound(tempF);	fs >> tempF; gS->SetDeathsPartsSum(tempF);	fs >> tempF; gS->SetLandPerPerson(tempF);
	
	fs >> gS->rS.landUnitPrice; fs >> gS->rS.deathsFromStarvation; fs >> gS->rS.deathsFromPlague; fs >> gS->rS.newPeopleNum;
	fs >> gS->rS.foodHarvested; fs >> gS->rS.foodFromLandUnit; fs >> gS->rS.foodSpoiledByRats;

	fs >> peopleFed; fs >> landSowed; fs >> populationAtRoundStart;
}


int main()
{
	const int GAMEOVER = 0;

	GameRules* gR = new GameRules();
	GameState* gS = new GameState(DEFAULT_INITIAL_ROUND, DEFAULT_FOOD, DEFAULT_POPULATION, DEFAULT_LAND);
	GameMode* gM = new GameMode(gR, gS);

	char input;
	std::cout << "Хотите загрузить сохранение? (y\\n)\n";
	while (true)
	{
		std::cin >> input;
		if (input == 'y')
		{
			gM->LoadGame(DEFAULT_SAVE_FILE_PATH);
			break;
		}
		if (input == 'n') break;
		std::cout << "Введите y (да) или n (нет).\n";
		std::cin.clear();
		std::cin.ignore(1000, '\n');
	}

	if (gS->GetRound() > 1) gM->ShowRoundStatistics();
	else gM->ShowInitialMessage();

	for (int round = gS->GetRound(); round < gR->roundsNum; round++)
	{
		gM->MakeRequests();
		ERoundOutcomes roundOutcome = gM->ProcessRound();
		gM->ShowRoundStatistics();

		if (roundOutcome != ERoundOutcomes::continuation) break;
	}

	return 0;
}
