/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_printf.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: akunegel <akunegel@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/04/19 14:52:20 by akunegel          #+#    #+#             */
/*   Updated: 2026/10/01 15:00:00 by elotana          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_printf.h"

int	ft_type(va_list *test, const char type)
{
	int	taille;

	taille = 0;
	if (type == 'c')
		taille += ft_c(va_arg(*test, int));
	else if (type == 's')
		taille += ft_s(va_arg(*test, char *));
	else if (type == 'p')
	{
		ft_putstr_fd("0x", 1);
		taille += ft_hexa_min(va_arg(*test, unsigned long long)) + 2;
	}
	else if (type == 'd')
		taille += ft_du(va_arg(*test, int));
	else if (type == 'i')
		taille += ft_du(va_arg(*test, int));
	else if (type == 'u')
		taille += ft_d(va_arg(*test, unsigned int));
	else if (type == 'x')
		taille += ft_hexa_min(va_arg(*test, unsigned int));
	else if (type == 'X')
		taille += ft_hex_maj(va_arg(*test, unsigned int));
	else if (type == '%')
		taille += ft_pourcent();
	return (taille);
}

int	ft_printf(const char *str, ...)
{
	int		i;
	int		taille;
	va_list	test;

	i = 0;
	taille = 0;
	va_start(test, str);
	while (str[i])
	{
		if (str[i] == '%')
		{
			taille += ft_type(&test, str[i + 1]);
			i++;
		}
		else
		{
			ft_putchar(str[i]);
			taille++;
		}
		i++;
	}
	va_end(test);
	return (taille);
}
