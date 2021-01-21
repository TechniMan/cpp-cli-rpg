#ifndef _ENEMY_HPP_
#define _ENEMY_HPP_

#include "Creature.hpp"

class Enemy : public Creature {
    private:
        unsigned int m_gold;

    public:
        Enemy(std::string& name, int gold, uint attackBonus, uint dodgeBonus, uint hitPoints);
        ~Enemy();

        unsigned int GoldWorth() const;
};

#endif // _ENEMY_HPP_
