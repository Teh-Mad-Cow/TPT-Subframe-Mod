#include "Tool.h"

#include "client/Client.h"
#include "Menu.h"

#include "gui/game/GameModel.h"
#include "gui/Style.h"
#include "gui/game/Brush.h"
#include "gui/interface/Window.h"
#include "gui/interface/Button.h"
#include "gui/interface/Label.h"
#include "gui/interface/Textbox.h"
#include "gui/interface/DropDown.h"
#include "gui/interface/Keys.h"
#include "gui/dialogues/ErrorMessage.h"

#include "simulation/GOLString.h"
#include "simulation/BuiltinGOL.h"
#include "simulation/Simulation.h"
#include "simulation/SimulationData.h"
#include "simulation/ElementClasses.h"

#include "graphics/Graphics.h"

#include <iostream>

void ParseFloatProperty(String value, float &out)
{
	if (value.EndsWith("C"))
	{
		float v = value.SubstrFromEnd(1).ToNumber<float>();
		out = v + 273.15;
	}
	else if(value.EndsWith("F"))
	{
		float v = value.SubstrFromEnd(1).ToNumber<float>();
		out = (v-32.0f)*5/9+273.15f;
	}
	else
	{
		out = value.ToNumber<float>();
	}
#ifdef DEBUG
	std::cout << "Got float value " << out << std::endl;
#endif
}

bool TryParseHexProperty(String value, int &out)
{
	if(value.length() > 2 && value.BeginsWith("0X"))
	{
		//0xC0FFEE
		out = value.Substr(2).ToNumber<unsigned int>(Format::Hex());
		return true;
	}
	else if(value.length() > 1 && value.BeginsWith("#"))
	{
		//#C0FFEE
		out = value.Substr(1).ToNumber<unsigned int>(Format::Hex());
		return true;
	}
	return false;
}

class PropertyWindow: public ui::Window
{
public:
	ui::DropDown * property;
	ui::Textbox * textField;
	PropertyTool * tool;
	Simulation *sim;
	std::vector<StructProperty> properties;
	PropertyWindow(PropertyTool *tool_, Simulation *sim);
	void SetProperty(bool warn);
	void OnDraw() override;
	void OnKeyPress(int key, int scan, bool repeat, bool shift, bool ctrl, bool alt) override;
	void OnTryExit(ExitMethod method) override;
	virtual ~PropertyWindow() {}
};

PropertyWindow::PropertyWindow(PropertyTool * tool_, Simulation *sim_):
ui::Window(ui::Point(-1, -1), ui::Point(200, 87)),
tool(tool_),
sim(sim_)
{
	std::vector<StructProperty> origProperties = Particle::GetProperties();
	properties.reserve(origProperties.size());
	static ByteString firstProperties[] = {
		"type", "ctype",
		"life", "temp",
		"tmp", "tmp2",
	};
	int numFirstProperties =
		sizeof(firstProperties) / sizeof(*firstProperties);
	for (int i = 0; i < numFirstProperties; i++)
	{
		for (int j = 0; j < int(origProperties.size()); j++)
		{
			if (origProperties[j].Name == firstProperties[i])
			{
				properties.push_back(origProperties[j]);
				break;
			}
		}
	}
	for (int i = 0; i < int(origProperties.size()); i++)
	{
		bool inFirstProperties = false;
		for (int j = 0; j < numFirstProperties; j++)
		{
			if (origProperties[i].Name == firstProperties[j])
			{
				inFirstProperties = true;
				break;
			}
		}
		if (inFirstProperties)
			continue;
		properties.push_back(origProperties[i]);
	}

	ui::Label * messageLabel = new ui::Label(ui::Point(4, 5), ui::Point(Size.X-8, 14), "Edit property");
	messageLabel->SetTextColour(style::Colour::InformationTitle);
	messageLabel->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;
	messageLabel->Appearance.VerticalAlign = ui::Appearance::AlignTop;
	AddComponent(messageLabel);

	ui::Button * okayButton = new ui::Button(ui::Point(0, Size.Y-17), ui::Point(Size.X, 17), "OK");
	okayButton->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;
	okayButton->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
	okayButton->Appearance.BorderInactive = ui::Colour(200, 200, 200);
	okayButton->SetActionCallback({ [this] {
		if (textField->GetText().length())
		{
			CloseActiveWindow();
			SetProperty(true);
			SelfDestruct();
		}
	} });
	AddComponent(okayButton);
	SetOkayButton(okayButton);

	property = new ui::DropDown(ui::Point(8, 25), ui::Point(Size.X-16, 16));
	property->SetActionCallback({ [this] { FocusComponent(textField); } });
	AddComponent(property);
	for (int i = 0; i < int(properties.size()); i++)
	{
		property->AddOption(std::pair<String, int>(properties[i].Name.FromAscii(), i));
	}
	property->SetOption(Client::Ref().GetPrefInteger("Prop.Type", 0));

	textField = new ui::Textbox(ui::Point(8, 46), ui::Point(Size.X-16, 16), "", "[value]");
	textField->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;
	textField->Appearance.VerticalAlign = ui::Appearance::AlignMiddle;
	textField->SetText(Client::Ref().GetPrefString("Prop.Value", ""));
	AddComponent(textField);
	FocusComponent(textField);
	SetProperty(false);

	MakeActiveWindow();
}

void PropertyWindow::SetProperty(bool warn)
{
	tool->validProperty = false;
	if(property->GetOption().second!=-1 && textField->GetText().length() > 0)
	{
		tool->validProperty = true;
		String value = textField->GetText().ToUpper();
		try {
			switch(properties[property->GetOption().second].Type)
			{
				case StructProperty::Integer:
				case StructProperty::ParticleType:
				{
					int v;
					if (TryParseHexProperty(value, v))
					{
						// found value, nothing to do
					}
					else if(value.length() > 5 && value.BeginsWith("FILT:"))
					{
						// CRAY(FILT), e.g. filt:5
						v = value.Substr(5).ToNumber<unsigned int>();
						v = PMAP(v, PT_FILT);
					}
					else if(value.length() > 2 && properties[property->GetOption().second].Name == "ctype" && value.BeginsWith("C:"))
					{
						// 30th-bit handling, e.g. C:100
						if (TryParseHexProperty(value.Substr(2), v))
						{
							// found value, nothing to do
						}
						else
						{
							v = value.Substr(2).ToNumber<int>();
						}
						v = (v & 0x3FFFFFFF) | (1<<29);
					}
					else
					{
						// Try to parse as particle name
						v = sim->GetParticleType(value.ToUtf8());

						// Try to parse special GoL rules
						if (v == -1 && properties[property->GetOption().second].Name == "ctype")
						{
							if (value.length() > 1 && value.BeginsWith("B") && value.Contains("/"))
							{
								v = ParseGOLString(value);
								if (v == -1)
								{
									class InvalidGOLString : public std::exception
									{
									};
									throw InvalidGOLString();
								}
							}
							else
							{
								v = sim->GetParticleType(value.ToUtf8());
								if (v == -1)
								{
									for (auto *elementTool : tool->gameModel->GetMenuList()[SC_LIFE]->GetToolList())
									{
										if (elementTool && elementTool->GetName() == value)
										{
											v = ID(elementTool->GetToolID());
											break;
										}
									}
								}
							}
						}

						// Parse as plain number
						if (v == -1)
						{
							v = value.ToNumber<int>();
						}
					}

					if (properties[property->GetOption().second].Name == "type" && (v < 0 || v >= PT_NUM || !sim->elements[v].Enabled))
					{
						tool->validProperty = false;
						if (warn)
							new ErrorMessage("Could not set property", "Invalid particle type");
						return;
					}

#ifdef DEBUG
					std::cout << "Got int value " << v << std::endl;
#endif

					tool->propValue.Integer = v;
					break;
				}
				case StructProperty::UInteger:
				{
					unsigned int v;
					if(value.length() > 2 && value.BeginsWith("0X"))
					{
						//0xC0FFEE
						v = value.Substr(2).ToNumber<unsigned int>(Format::Hex());
					}
					else if(value.length() > 1 && value.BeginsWith("#"))
					{
						//#C0FFEE
						v = value.Substr(1).ToNumber<unsigned int>(Format::Hex());
					}
					else
					{
						v = value.ToNumber<unsigned int>();
					}
#ifdef DEBUG
					std::cout << "Got uint value " << v << std::endl;
#endif
					tool->propValue.UInteger = v;
					break;
				}
				case StructProperty::Float:
				{
					ParseFloatProperty(value, tool->propValue.Float);
				}
					break;
				default:
					tool->validProperty = false;
					if (warn)
						new ErrorMessage("Could not set property", "Invalid property");
					return;
			}
			tool->propOffset = properties[property->GetOption().second].Offset;
			tool->propType = properties[property->GetOption().second].Type;
			tool->changeType = properties[property->GetOption().second].Name == "type";
		} catch (const std::exception& ex) {
			tool->validProperty = false;
			Client::Ref().SetPref("Prop.Type", property->GetOption().second);
			Client::Ref().SetPrefUnicode("Prop.Value", String(""));
			if (warn)
				new ErrorMessage("Could not set property", "Invalid value provided");
			return;
		}
		Client::Ref().SetPref("Prop.Type", property->GetOption().second);
		Client::Ref().SetPrefUnicode("Prop.Value", textField->GetText());
	}
}

void PropertyWindow::OnTryExit(ExitMethod method)
{
	CloseActiveWindow();
	SelfDestruct();
}

void PropertyWindow::OnDraw()
{
	Graphics * g = GetGraphics();

	g->clearrect(Position.X-2, Position.Y-2, Size.X+3, Size.Y+3);
	g->drawrect(Position.X, Position.Y, Size.X, Size.Y, 200, 200, 200, 255);
}

void PropertyWindow::OnKeyPress(int key, int scan, bool repeat, bool shift, bool ctrl, bool alt)
{
	if (key == SDLK_UP)
		property->SetOption(property->GetOption().second-1);
	else if (key == SDLK_DOWN)
		property->SetOption(property->GetOption().second+1);
}

void PropertyTool::OpenWindow(Simulation *sim)
{
	new PropertyWindow(this, sim);
}

void PropertyTool::SetProperty(Simulation *sim, ui::Point position)
{
	if(position.X<0 || position.X>XRES || position.Y<0 || position.Y>YRES || !validProperty)
		return;
	int i = sim->pmap[position.Y][position.X];
	if(!i)
		i = sim->photons[position.Y][position.X];
	if(!i)
		return;

	if (changeType)
	{
		sim->part_change_type(ID(i), int(sim->parts[ID(i)].x+0.5f), int(sim->parts[ID(i)].y+0.5f), propValue.Integer);
		return;
	}

	switch (propType)
	{
		case StructProperty::Float:
			*((float*)(((char*)&sim->parts[ID(i)])+propOffset)) = propValue.Float;
			break;
		case StructProperty::ParticleType:
		case StructProperty::Integer:
			if (propOffset == offsetof(Particle, ctype) && (sim->parts[ID(i)].type == PT_FILT || sim->parts[ID(i)].type == PT_BRAY || sim->parts[ID(i)].type == PT_PHOT))
			{
				propValue.Integer &= 0x3FFFFFFF;
			}
			*((int*)(((char*)&sim->parts[ID(i)])+propOffset)) = propValue.Integer;
			break;
		case StructProperty::UInteger:
			*((unsigned int*)(((char*)&sim->parts[ID(i)])+propOffset)) = propValue.UInteger;
			break;
		default:
			break;
	}
}

void PropertyTool::Draw(Simulation *sim, Brush *cBrush, ui::Point position)
{
	if(cBrush)
	{
		int radiusX = cBrush->GetRadius().X, radiusY = cBrush->GetRadius().Y, sizeX = cBrush->GetSize().X, sizeY = cBrush->GetSize().Y;
		unsigned char *bitmap = cBrush->GetBitmap();
		for(int y = 0; y < sizeY; y++)
			for(int x = 0; x < sizeX; x++)
				if(bitmap[(y*sizeX)+x] && (position.X+(x-radiusX) >= 0 && position.Y+(y-radiusY) >= 0 && position.X+(x-radiusX) < XRES && position.Y+(y-radiusY) < YRES))
					SetProperty(sim, ui::Point(position.X+(x-radiusX), position.Y+(y-radiusY)));
	}
}

void PropertyTool::DrawLine(Simulation *sim, Brush *cBrush, ui::Point position, ui::Point position2, bool dragging)
{
	int x1 = position.X, y1 = position.Y, x2 = position2.X, y2 = position2.Y;
	bool reverseXY = abs(y2-y1) > abs(x2-x1);
	int x, y, dx, dy, sy, rx = cBrush->GetRadius().X, ry = cBrush->GetRadius().Y;
	float e = 0.0f, de;
	if (reverseXY)
	{
		y = x1;
		x1 = y1;
		y1 = y;
		y = x2;
		x2 = y2;
		y2 = y;
	}
	if (x1 > x2)
	{
		y = x1;
		x1 = x2;
		x2 = y;
		y = y1;
		y1 = y2;
		y2 = y;
	}
	dx = x2 - x1;
	dy = abs(y2 - y1);
	if (dx)
		de = dy/(float)dx;
	else
		de = 0.0f;
	y = y1;
	sy = (y1<y2) ? 1 : -1;
	for (x=x1; x<=x2; x++)
	{
		if (reverseXY)
			Draw(sim, cBrush, ui::Point(y, x));
		else
			Draw(sim, cBrush, ui::Point(x, y));
		e += de;
		if (e >= 0.5f)
		{
			y += sy;
			if (!(rx+ry) && ((y1<y2) ? (y<=y2) : (y>=y2)))
			{
				if (reverseXY)
					Draw(sim, cBrush, ui::Point(y, x));
				else
					Draw(sim, cBrush, ui::Point(x, y));
			}
			e -= 1.0f;
		}
	}
}

void PropertyTool::DrawRect(Simulation *sim, Brush *cBrush, ui::Point position, ui::Point position2)
{
	int x1 = position.X, y1 = position.Y, x2 = position2.X, y2 = position2.Y;
	int i, j;
	if (x1>x2)
	{
		i = x2;
		x2 = x1;
		x1 = i;
	}
	if (y1>y2)
	{
		j = y2;
		y2 = y1;
		y1 = j;
	}
	for (j=y1; j<=y2; j++)
		for (i=x1; i<=x2; i++)
			SetProperty(sim, ui::Point(i, j));
}

void PropertyTool::DrawFill(Simulation *sim, Brush *cBrush, ui::Point position)
{
	if (validProperty)
		sim->flood_prop(position.X, position.Y, propOffset, propValue, propType);
}
