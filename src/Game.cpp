#include "Game.hpp"

#include <cmath>

std::string GAME_SAVES_DIRECTORY = "saves/";



Game::Game() {
	m_logger = new Logger();
	m_rng = pcg32();
}



Game::~Game() {
	if (m_logger != nullptr) delete m_logger;
	if (m_enemy != nullptr) delete m_enemy;
	if (m_player != nullptr) delete m_player;
}



void Game::InitialiseCombat() {
	m_adventureDepth++;
	unsigned int a = (unsigned int)(floorf((float)m_adventureDepth * 0.7f));
	std::string kobold("kobold");
	m_enemy = new Enemy(kobold, AvailableWeapons.at("Dagger"), a, a, a, 2 * m_adventureDepth, 3 * m_adventureDepth);
	m_enemy->Heal(0);

	m_logger->WriteLine("You've come across a " + m_enemy->GetName() + "!");
	m_logger->WriteLine("You could try to {attack} it, check your and its {status}, or try and {run} away back to safety!");
}



void Game::EndCombat() {
	m_player->Reward(m_enemy->GoldWorth(), m_enemy->ExpWorth());
	m_logger->WriteLine("You have been rewarded " + std::to_string(m_enemy->GoldWorth()) + "g and " + std::to_string(m_enemy->ExpWorth()) + "exp! Spend them in town.");
}

void Game::InitialiseShop() {
	if (m_townShop == nullptr) {
		m_townShop = new Shop();
		auto shopInv = std::map<ITEMS, uint>();
		shopInv.emplace(ITEMS::HEALTH_POTION, 10U);
		m_townShop->SetInventoryAndPrices(shopInv);
	}
}



void Game::TransitionState(GAMESTATE stateTo) {
	m_logger->WriteLine();
	
	switch (stateTo) {
		case GAMESTATE::TITLE:
			break;
		
		case GAMESTATE::MENU:
			m_gameState = GAMESTATE::MENU;
			m_logger->WriteLine("Menu: From here, you can start a {new} game, {load} another character, or {exit}! At any time, {help} will tell you the available commands.");
			break;
		
		case GAMESTATE::TOWN:
			m_gameState = GAMESTATE::TOWN;
			InitialiseShop();
			m_logger->WriteLine("Welcome to town, " + m_player->GetName() + "!");
			m_logger->WriteLine("Here in the main square, you could {rest} at the Inn or go {travel}ing in the wilderness in search of adventure and fortune. You can also check your {status}.");
			break;
		
		case GAMESTATE::COMBAT:
			m_gameState = GAMESTATE::COMBAT;
			m_adventureDepth = 0; // start at 0; it is incremented by InitCombat()
			m_logger->WriteLine("You venture out into the wilderness.");
			InitialiseCombat();
			break;
		
		default:
			break;
	}
}



// returns true to keep playing, or false to end the game loop
bool Game::ProcessCommand(std::string command, std::string mainArg, std::string fullArg) {
	// quit
	if (command == "exit" || command == "quit") {
		if (m_gameState == GAMESTATE::TOWN) {
			m_logger->WriteLine("Quit game in progress. See you, " + m_player->GetName() + "!");
			return false;
		}
		else if (m_gameState == GAMESTATE::MENU) {
			m_logger->WriteLine("Bye!");
			return false;
		}
		else {
			m_logger->WriteCommandNotFound(command);
		}
	}

	// help
	else if (command == "help") {
		m_logger->WriteHelp(m_gameState);
	}

	// else, play the game!

	GAMESTATE transitionState = m_gameState;
	// process input based on game state
	switch (m_gameState)
	{
		case GAMESTATE::TITLE:
			m_gameState = GAMESTATE::MENU;
			m_logger->WriteHelp(m_gameState);
			break;
		


		case GAMESTATE::MENU:
			if (command == "new") {
				if (fullArg.length() > 0) {
					m_player = new Player(fullArg);
					m_player->Heal(0);
					transitionState = GAMESTATE::TOWN;
				}
				else {
					m_logger->WriteLine("Please specify a name for the brave new hero!");
				}
			}
			else if (command == "load") {
				if (fullArg.length() > 0) {
					Player * loadPlayer = new Player(fullArg);
					if (Player::Load(GAME_SAVES_DIRECTORY + fullArg + ".sav", *loadPlayer)) {
						m_player = loadPlayer;
						transitionState = GAMESTATE::TOWN;
					}
					else {
						m_logger->WriteLine("Unfortunately " + fullArg + " couldn't be loaded. Did you spell it right?");
					}
				}
				else {
					m_logger->WriteLine("Please specify a name for the hero you wish to awaken!");
				}
			}
			else if (command == "about") {
				m_logger->WriteLine("GitHub: https://github.com/techniman/cpp-cli-rpg");
				m_logger->WriteLine("Programmer: Will Thomas, technimanx@googlemail.com, https://github.com/techniman");
			}
			else if (command != "help") {
				m_logger->WriteCommandNotFound(command);
			}
			break;



		case GAMESTATE::TOWN:
			if (command == "rest") {
				// player try to pay for inn
				// else annoyed innkeeper grr
				// if player is super injured and also unable to pay, then they are stuck forever to die on the streets. Whoops
				m_player->Heal(0);
				m_logger->WriteLine(m_player->GetName() + "'s health replenished through rest! HP: " + m_player->CurrentHealth());
				m_logger->WriteLine("Saving to " + GAME_SAVES_DIRECTORY + m_player->GetName() + ".sav ...");
				if (m_player->Save(GAME_SAVES_DIRECTORY + m_player->GetName() + ".sav")) {
					m_logger->WriteLine("Saved " + m_player->GetName() + " to file!");
				}
				else {
					std::string command = "mkdir " + GAME_SAVES_DIRECTORY;
					system(command.c_str());
					if (m_player->Save(GAME_SAVES_DIRECTORY + m_player->GetName() + ".sav")) {
						m_logger->WriteLine("Saved " + m_player->GetName() + " to file!");
					}
					else {
						m_logger->WriteLine("Unfortunately there was an error trying to save.");
					}
				}
				if (/*m_player->GetExp() > m_player->LevelUpCost()*/false) {
					m_logger->WriteLine("It looks like you have enough exp to {levelup}!");
				}
			}

			else if (command == "travel") {
				if (m_player->HPPercentage() >= 0.5f) {
					transitionState = GAMESTATE::COMBAT;
				}
				else {
					m_logger->WriteLine("You should think about resting up, first! HP: " + m_player->CurrentHealth());
				}
			}

			else if (command == "status") {
				m_logger->WriteLine(m_player->ToString());
				m_logger->WriteLine("The next level up will cost " + std::to_string(m_player->LevelUpCost()) + "exp.");

				if (mainArg == "help") {
					m_logger->WriteLine("str: affects your attack damage");
					m_logger->WriteLine("dex: affects your attack hit chance and dodge chance");
					m_logger->WriteLine("con: affects your maximum hit points");
				}
			}

			else if (command == "levelup") {
				auto abi = mainArg.substr(0, 3);
				if (abi == "str")
				{
					if (m_player->LevelUp(CREATURE_ABILITIES_STRENGTH)) {
						m_logger->WriteLine("Your strength score has increased!");
					}
					else {
						m_logger->WriteLine("You need at least " + std::to_string(m_player->LevelUpCost()) + "exp to level up an ability.");
					}
				}
				else if (abi == "dex") {
					if (m_player->LevelUp(CREATURE_ABILITIES_DEXTERITY)) {
						m_logger->WriteLine("Your dexterity score has increased!");
					}
					else {
						m_logger->WriteLine("You need at least " + std::to_string(m_player->LevelUpCost()) + "exp to level up an ability.");
					}
				}
				else if (abi == "con") {
					if (m_player->LevelUp(CREATURE_ABILITIES_CONSTITUTION)) {
						m_logger->WriteLine("Your constitution score has increased!");
					}
					else {
						m_logger->WriteLine("You need at least " + std::to_string(m_player->LevelUpCost()) + "exp to level up an ability.");
					}
				}
				else {
					m_logger->WriteLine("I'm afraid you don't have a '" + mainArg + "' ability. Try:");
					m_logger->WriteLine("str: affects your attack damage");
					m_logger->WriteLine("dex: affects your attack hit chance and dodge chance");
					m_logger->WriteLine("con: affects your maximum hit points");
				}
			}

			else if (command == "inventory") {
				// list player inventory
				m_logger->WriteInventory(m_player->GetInventory());
			}

			else if (command == "shop") {
				if (mainArg == "") {
					m_logger->WriteLine("\"Good day to you, sir! May I interest you in {buy}in' somethin' from me {inventory}?\"");
					m_logger->WriteLine("Please specify {shop} followed by a sub-command.");
				}
				else {
					std::string itemName = fullArg.length() > mainArg.length() ? fullArg.substr(mainArg.length() + 1) : "";
					if (mainArg == "inventory") {
						m_logger->WriteInventory(m_townShop->GetInventory());
					}
					else if (mainArg == "buy") {
						auto item = Item::GetByName(itemName);
						uint itemCost = m_townShop->GetCostOf(item);
						if (itemCost == 0) {
							m_logger->WriteLine("\"'Fraid I en't got any o' those in stock at the moment.\"");
						}
						else {
							if (m_player->Pay(itemCost)) {
								m_player->Give(item);
								m_logger->WriteLine("\"Pleasure doin' business. Stay safe out there!\"");
							}
							else {
								m_logger->WriteLine("\"Oi! Ye can't afford this! Git outta here 'til ye gets yerself more coin.\"");
							}
						}
					}
					else if (mainArg == "sell") {
						// TODO: allow player to sell items for money?
						m_logger->WriteLine("Not implemented yet!");
					}
				}
			}

			else if (command != "help" && command != "exit" && command != "quit") {
				m_logger->WriteCommandNotFound(command);
			}
			break;



		case GAMESTATE::COMBAT:
			bool takenTurn;
			takenTurn = false;

			if (command == "attack") {
				// make attack roll
				unsigned int roll = pcg_extras::bounded_rand(m_rng, 20);
				unsigned int attackRoll = roll + m_player->AttackBonus();
				if (attackRoll >= m_enemy->Defense()) {
					int dmg = m_player->AttackDamage(m_rng);
					m_enemy->Damage(dmg);
					m_logger->WriteLine("You hit and dealt " + std::to_string(dmg) + " damage to the " + m_enemy->GetName() + ".");
				}
				else {
					m_logger->WriteLine("Your attack missed!");
				}
				takenTurn = true;

				// if enemy is defeated, ask to carry on in the wilderness or return home
				if (!m_enemy->IsAlive()) {
					EndCombat();
					takenTurn = false;
					m_logger->WriteLine("You defeated " + m_enemy->GetName() + "! Would you like to continue? Greater dangers and also rewards await you.");
					m_logger->Write("y/n > ");
					
					std::string confirm;
					if (std::getline(std::cin, confirm)) {
						if (confirm.at(0) == 'y' || confirm.at(0) == 'Y') {
							InitialiseCombat();
						}
						else {
							m_logger->WriteLine("You return to town safely.");
							transitionState = GAMESTATE::TOWN;
						}
					}
				}
			}

			else if (command == "cast") {
				// cast a spell zap!
				// check spellbook for arg1
				// if exists, do it and end turn
				//takenTurn = true;
				// else report failure
				m_logger->WriteLine("Not yet implemented!");
			}

			else if (command == "consume") {
				// consume a thing nom!
				if (m_player->Consume(Item::GetByName(mainArg), *m_enemy)) {
					m_logger->WriteLine("Mmm, " + mainArg + "! Delicious.");
					takenTurn = true;
				}
				else {
					m_logger->WriteLine("I'm afraid you can't consume " + mainArg + ". Do you have one in your {inventory}?");
				}
			}

			else if (command == "status") {
				m_logger->WriteLine(m_player->GetName() + "'s HP: " + m_player->CurrentHealth() + " and " + m_enemy->GetName() + "'s HP: " + m_enemy->CurrentHealth());
				m_logger->WriteLine(m_player->ToString());
				m_logger->WriteLine(m_enemy->ToString());
			}

			else if (command == "run") {
				// roll under a certain chance to successfully run away
				unsigned int roll = pcg_extras::bounded_rand(m_rng, 20);
				if (roll < 10) {
					m_logger->WriteLine("You try to get away... but failed!");
				}
				else {
					m_logger->WriteLine("You try to get away... and succeeded!");
					m_logger->WriteLine("The " + m_enemy->GetName() + " takes a final swing at you!");
					transitionState = GAMESTATE::TOWN;
				}
				takenTurn = true;
			}

			else if (command != "help") {
				m_logger->WriteCommandNotFound(command);
			}

			// the enemy takes a turn after the player
			if (takenTurn) {
				unsigned int roll = pcg_extras::bounded_rand(m_rng, 20);
				unsigned int attackRoll = roll + m_enemy->AttackBonus();
				if (attackRoll >= m_player->Defense()) {
					int dmg = m_enemy->AttackDamage(m_rng);
					m_player->Damage(dmg);
					m_logger->WriteLine("The " + m_enemy->GetName() + " hit you for " + std::to_string(dmg) + " damage!");
				}
				else {
					m_logger->WriteLine("The " + m_enemy->GetName() + "\'s attack missed you.");
				}
				
				// check if player died
				if (!m_player->IsAlive()) {
					m_logger->WriteLine();
					m_logger->WriteLine("The brave hero, " + m_player->GetName() + ", has fallen to a " + m_enemy->GetName() + "!");
					m_logger->WriteLine();
					m_logger->WriteLine("Thank you for playing! You have been returned to the main menu.");
					m_logger->WriteLine();
					transitionState = GAMESTATE::MENU;
				}
				// else report player and enemy statuses
				else {
					m_logger->WriteLine(m_player->GetName() + "'s HP: " + m_player->CurrentHealth() + " and " + m_enemy->GetName() + "'s HP: " + m_enemy->CurrentHealth());
				}
			}
			break;

		default:
			m_logger->WriteError("Game::ProcessCommand::INVALID_GAMESTATE");
			transitionState = GAMESTATE::MENU;
			break;
	}



	if (transitionState != m_gameState)
	{
		TransitionState((GAMESTATE)transitionState);
	}
	return true;
}
