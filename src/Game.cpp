#include "../include/Game.hpp"

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
	m_enemy = new Enemy("kobold", (int)m_adventureDepth - 2, 2 + m_adventureDepth, 0 + m_adventureDepth, 2 * m_adventureDepth, 2 * m_adventureDepth);
	m_enemy->Heal(0);

	m_logger->WriteLine("You've come across a " + m_enemy->GetName() + "!");
}

void Game::EndCombat() {
	m_player->Reward(m_enemy->GoldWorth(), m_enemy->ExpWorth());
	m_logger->WriteLine("You have been rewarded " + std::to_string(m_enemy->GoldWorth()) + "g and " + std::to_string(m_enemy->ExpWorth()) + "exp! Spend them in town.");
}

bool Game::ProcessCommand(std::string command, std::string mainArg, std::string fullArg) {
	// quit
	if (command == "exit" || command == "quit") {
		if (m_gameState == GAMESTATE_TOWN) {
			m_gameState = GAMESTATE_MENU;
			m_logger->WriteLine("Quit game. Goodbye, " + m_player->GetName() + "!");
		}
		else if (m_gameState == GAMESTATE_MENU) {
			m_logger->WriteLine("Bye!");
			return false;
		}
		else {
			//m_logger->WriteCommandNotFound(command);
		}
	}

	// help
	else if (command == "help") {
		m_logger->WriteHelp(m_gameState);
	}

	// else, play the game!

	// process input based on game state
	switch (m_gameState)
	{
		case GAMESTATE_TITLE:
			m_gameState = GAMESTATE_MENU;
			m_logger->WriteHelp(m_gameState);
			break;
		
		case GAMESTATE_MENU:
			if (command == "new") {
				if (fullArg.length() > 0) {
					m_player = new Player(fullArg);
					m_player->Heal(0);
					m_gameState = GAMESTATE_TOWN;
					m_logger->WriteLine("Welcome to town, " + m_player->GetName() +"!");
					m_logger->WriteLine("Here in the main square, you could {rest} at the Inn or go {travel}ing in the wilderness in search of adventure and fortunes. And of course you can {exit}.");
				}
				else {
					m_logger->WriteLine("Please specify a name for the brave new hero!");
				}
			}
			else if (command == "load") {
				if (fullArg.length() > 0) {
					m_logger->WriteLine("Not yet implemented!");
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

		case GAMESTATE_TOWN:
			if (command == "rest") {
				// player try to pay for inn
				// else annoyed innkeeper grr
				// if player is super injured and also unable to pay, then they are stuck forever to die on the streets. Whoops
				m_player->Heal(0);
				m_logger->WriteLine(m_player->GetName() + "'s health replenished through rest! HP: " + m_player->CurrentHealth());
				m_logger->WriteLine("Saving is not yet implemented!");
			}

			else if (command == "travel") {
				if (m_player->HPPercentage() >= 0.2f) {
					m_gameState = GAMESTATE_COMBAT;
					m_adventureDepth = 0; // start at 0; it is incremented by InitCombat()
					m_logger->WriteLine("You venture out into the wilderness.");
					InitialiseCombat();
				}
				else {
					m_logger->WriteLine("You should think about resting up, first! HP: " + m_player->CurrentHealth());
				}
			}

			else if (command == "status") {
				m_logger->WriteLine(m_player->ToString());

				if (mainArg == "help") {
					m_logger->WriteLine("str: affects your attack damage");
					m_logger->WriteLine("dex: affects your attack hit chance and dodge chance");
					m_logger->WriteLine("con: affects your maximum hit points");
				}
			}

			else if (command == "levelup") {
				if (mainArg == "str" || mainArg == "strength") {
					if (m_player->LevelUp(CREATURE_ABILITIES_STRENGTH)) {
						m_logger->WriteLine("Your strength score has increased!");
					}
					else {
						m_logger->WriteLine("You need at least " + std::to_string(LEVELUP_COST) + "exp to level up an ability.");
					}
				}
				else if (mainArg == "dex" || mainArg == "dexterity") {
					if (m_player->LevelUp(CREATURE_ABILITIES_DEXTERITY)) {
						m_logger->WriteLine("Your dexterity score has increased!");
					}
					else {
						m_logger->WriteLine("You need at least " + std::to_string(LEVELUP_COST) + "exp to level up an ability.");
					}
				}
				else if (mainArg == "con" || mainArg == "constitution") {
					if (m_player->LevelUp(CREATURE_ABILITIES_CONSTITUTION)) {
						m_logger->WriteLine("Your constitution score has increased!");
					}
					else {
						m_logger->WriteLine("You need at least " + std::to_string(LEVELUP_COST) + "exp to level up an ability.");
					}
				}
				else {
					m_logger->WriteLine("I'm afraid you don't have a '" + mainArg + "' skill. Try:");
					m_logger->WriteLine("str: affects your attack damage");
					m_logger->WriteLine("dex: affects your attack hit chance and dodge chance");
					m_logger->WriteLine("con: affects your maximum hit points");
				}
			}

			else if (command != "help" && command != "exit" && command != "quit") {
				m_logger->WriteCommandNotFound(command);
			}
			break;

		case GAMESTATE_COMBAT:
			bool takenTurn;
			takenTurn = false;

			if (command == "attack") {
				// make attack roll
				unsigned int roll = pcg_extras::bounded_rand(m_rng, 20);
				unsigned int attackRoll = roll + m_player->AttackBonus();
				if (attackRoll >= m_enemy->Defense()) {
					m_enemy->Damage(m_player->AttackDamage());
					m_logger->WriteLine("You hit and dealt " + std::to_string(m_player->AttackDamage()) + " damage to the " + m_enemy->GetName() + ".");
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
							m_gameState = GAMESTATE_TOWN;
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
				// check inventory for arg1
				// if exists, consume it and end turn
				//takenTurn = true;
				// else report failure
				m_logger->WriteLine("Not yet implemented!");
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
					m_gameState = GAMESTATE_TOWN;
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
					m_player->Damage(m_enemy->AttackDamage());
					m_logger->WriteLine("The " + m_enemy->GetName() + " hit you for " + std::to_string(m_enemy->AttackDamage()) + " damage!");
				}
				else {
					m_logger->WriteLine("The " + m_enemy->GetName() + "\'s attack missed you.");
				}
				
				// check if player died
				if (!m_player->IsAlive()) {
					m_logger->WriteLine("The brave hero, " + m_player->GetName() + ", has fallen to a " + m_enemy->GetName() + "!");
					
					m_gameState = GAMESTATE_MENU;
					m_logger->WriteLine("Thank you for playing! You have been returned to the main menu.");
				}
				// else report player and enemy statuses
				else {
					m_logger->WriteLine(m_player->GetName() + "'s HP: " + m_player->CurrentHealth() + " and " + m_enemy->GetName() + "'s HP: " + m_enemy->CurrentHealth());
				}
			}
			break;

		default:
			m_logger->WriteError("INVALID_GAMESTATE");
			m_gameState = GAMESTATE_MENU;
			break;
	}
	return true;
}
