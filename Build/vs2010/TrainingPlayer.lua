function createPlayer()
	bodyDef.type = b2_dynamicBody;
	bodyDef.position.Set(2,2);
	bodyDef.fixedRotation=true;
	playerBody = m_world:CreateBody(bodyDef);

	polygonShape.SetAsBox(.76f,1.28f);
	playerBody->CreateFixture(polygonShape,1.0);

	b2Vec2 center(0,-1.28f);
	polygonShape.SetAsBox(.76f,.2f, center, 0);
	b2FixtureDef fixtureDef;
	fixtureDef.shape = &polygonShape;
	fixtureDef.filter.categoryBits=playerFeetBits;
	playerFeet = playerBody:CreateFixture(&fixtureDef);
	-- body
end